#include "dxvnpu_backend.hpp"

#include <dxvnpu/dxvnpu_api.h>
#include <gst/gst.h>
#include <climits>
#include <cstring>

#define GST_CAT_DEFAULT inference_backend_cat
GST_DEBUG_CATEGORY_EXTERN(inference_backend_cat);

DxvnpuBackend::~DxvnpuBackend() {
    Flush();
    devices_.clear();
}

int DxvnpuBackend::get_max_wait_ms() {
    const char *env = g_getenv("DXVNPU_INFER_TIMEOUT_MS");
    if (env) {
        int val = atoi(env);
        if (val < 0) return INT_MAX;   // -1 = disable timeout (for debugging)
        if (val > 0) return val;
    }
    return 3000;  // default 3 seconds
}

bool DxvnpuBackend::Init(const InferBackendOptions& options) {
    if (options.model_path.empty() ||
        !g_file_test(options.model_path.c_str(), G_FILE_TEST_IS_REGULAR)) {
        GST_ERROR("DxvnpuBackend: model file not found: %s",
                  options.model_path.c_str());
        return false;
    }

    dxvnpu::InferenceConfig cfg;
    cfg.model_path = options.model_path;
    cfg.useORT = options.use_ort;

    if (options.device_id >= 0) {
        auto ctx = std::make_unique<DeviceContext>();
        ctx->device_id = options.device_id;
        try {
            ctx->module = std::make_unique<dxvnpu::InferenceModule>(cfg, options.device_id);
        } catch (const std::exception& e) {
            GST_ERROR("failed to create InferenceModule on device %d: %s",
                      options.device_id, e.what());
            return false;
        }
        if (!ctx->module->IsStarted()) {
            GST_ERROR("InferenceModule failed to start on device %d", options.device_id);
            return false;
        }
        devices_.push_back(std::move(ctx));
    } else {
        size_t count = dxvnpu::GetDeviceCount();
        if (count == 0) {
            GST_ERROR("no VNPU devices found");
            return false;
        }

        GST_DEBUG("found %zu VNPU device(s), initializing...", count);

        for (size_t i = 0; i < count; i++) {
            auto ctx = std::make_unique<DeviceContext>();
            ctx->device_id = static_cast<int>(i);
            try {
                ctx->module = std::make_unique<dxvnpu::InferenceModule>(cfg, ctx->device_id);
            } catch (const std::exception& e) {
                GST_WARNING("failed to create InferenceModule on device %zu: %s", i, e.what());
                continue;
            }
            if (!ctx->module->IsStarted()) {
                GST_WARNING("InferenceModule failed to start on device %zu, skipping", i);
                continue;
            }
            devices_.push_back(std::move(ctx));
        }

        if (devices_.empty()) {
            GST_ERROR("no VNPU devices could be initialized");
            return false;
        }
    }

    device_count_ = devices_.size();
    input_size_ = devices_[0]->module->GetInputSize();
    output_size_ = devices_[0]->module->GetOutputSize();

    GST_INFO("initialized %zu device(s) (input_size=%zu, output_size=%zu)",
             device_count_, input_size_, output_size_);
    for (size_t i = 0; i < device_count_; i++) {
        GST_INFO("  device[%zu]: device_id=%d", i, devices_[i]->device_id);
    }
    return true;
}

bool DxvnpuBackend::Put(void* input_ptr, void* output_ptr) {
    if (flushed_.load()) return false;

    size_t idx = next_device_ % device_count_;
    auto& dev = devices_[idx];

    auto input = std::make_shared<dxvnpu::DXTensors>();
    input->data_size = input_size_;
    input->data = std::shared_ptr<void>(input_ptr, [](void*) {});

    auto status = dev->module->PutTensor(input, -1);
    if (status != dxvnpu::Status::OK) {
        GST_ERROR("PutTensor failed on device %d: %s",
                  dev->device_id, dxvnpu::getStatusString(status));
        return false;
    }

    size_t pending;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        pending_requests_.push({idx, output_ptr, put_count_.load(),
                                std::chrono::steady_clock::now()});
        pending = pending_requests_.size();
    }

    size_t req_id = put_count_.fetch_add(1);
    next_device_++;
    GST_TRACE("Put req_id=%zu to device[%zu] (device_id=%d, pending=%zu, total_put=%zu)",
              req_id, idx, dev->device_id, pending, req_id + 1);
    return true;
}

bool DxvnpuBackend::Get(dxs::DXTensors& output) {
    PendingRequest req;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (pending_requests_.empty()) {
            GST_WARNING("Get called with no pending requests");
            return false;
        }
        req = pending_requests_.front();
        pending_requests_.pop();
    }

    auto& dev = devices_[req.device_idx];
    const int max_wait = get_max_wait_ms();
    int elapsed = 0;

    while (!flushed_.load()) {
        auto result = dev->module->GetTensor(DRAIN_TIMEOUT_MS);
        if (result) {
            size_t seq = get_count_.fetch_add(1);
            GST_TRACE("Got seq=%zu from device[%zu] (device_id=%d, total_put=%zu, total_get=%zu)",
                      seq, req.device_idx, dev->device_id, put_count_.load(), get_count_.load());

            if (req.output_ptr && result->getRawData() && result->data_size > 0) {
                memcpy(req.output_ptr, result->getRawData(),
                       std::min(result->data_size, output_size_));
            }

            for (const auto& meta : result->tensor_list) {
                dxs::DXTensor t;
                t._name = meta.name;
                t._shape = meta.shape;
                t._type = static_cast<dxs::DataType>(meta.data_type);
                t._elemSize = meta.elem_size;
                if (req.output_ptr) {
                    t._data = static_cast<uint8_t*>(req.output_ptr) + meta.data_offset;
                }
                output._tensors.push_back(t);
            }
            return true;
        }

        elapsed += DRAIN_TIMEOUT_MS;
        if (elapsed >= max_wait) {
            auto wait_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - req.put_time).count();
            GST_WARNING("Get SKIP: seq=%zu, device[%zu] (device_id=%d), "
                        "polled=%dms, since_put=%ldms, total_put=%zu, total_get=%zu",
                        req.seq_num, req.device_idx, dev->device_id,
                        elapsed, (long)wait_ms, put_count_.load(), get_count_.load());
            get_count_.fetch_add(1);
            return false;  // timeout -- caller will skip this frame
        }
    }

    GST_WARNING("Get aborted (flushed), seq=%zu, total_put=%zu, total_get=%zu",
                req.seq_num, put_count_.load(), get_count_.load());
    return false;
}

void DxvnpuBackend::Flush() {
    flushed_.store(true);
}

void DxvnpuBackend::Reset() {
    // drain pending results from InferenceModule
    size_t drained = 0;
    for (auto& dev : devices_) {
        while (true) {
            auto result = dev->module->GetTensor(DRAIN_TIMEOUT_MS);
            if (!result) break;
            drained++;
        }
    }
    if (drained > 0) {
        GST_INFO("Reset: drained %zu pending results from InferenceModule(s)", drained);
    }

    {
        std::lock_guard<std::mutex> lock(mutex_);
        while (!pending_requests_.empty()) pending_requests_.pop();
    }
    next_device_ = 0;
    put_count_ = 0;
    get_count_ = 0;
    flushed_.store(false);
}

size_t DxvnpuBackend::GetOutputBufferSize() const {
    return output_size_;
}
