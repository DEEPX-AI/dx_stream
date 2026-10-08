#include "dxvnpu_backend.hpp"

#include <gst/gst.h>
#include <climits>
#include <cstring>

#define GST_CAT_DEFAULT vnpu_backend_cat
GST_DEBUG_CATEGORY_STATIC(vnpu_backend_cat);

static void ensure_vnpu_backend_debug_category() {
    static gsize initialized = 0;
    if (g_once_init_enter(&initialized)) {
        GST_DEBUG_CATEGORY_INIT(vnpu_backend_cat, "dxvnpu-backend", 0,
                                "DXVNPU inference backend");
        g_once_init_leave(&initialized, 1);
    }
}

DxvnpuBackend::~DxvnpuBackend() {
    Flush();
    devices_.clear();
}

int DxvnpuBackend::get_max_wait_ms() {
    const char* env = g_getenv("DXVNPU_INFER_TIMEOUT_MS");
    if (env) {
        int val = atoi(env);
        if (val < 0) return INT_MAX;
        if (val > 0) return val;
    }
    return 3000;
}

bool DxvnpuBackend::Init(const InferBackendOptions& options) {
    ensure_vnpu_backend_debug_category();
    if (options.model_path.empty() ||
        !g_file_test(options.model_path.c_str(), G_FILE_TEST_IS_REGULAR)) {
        GST_ERROR("DxvnpuBackend: model file not found: %s",
                  options.model_path.c_str());
        return false;
    }

    uint64_t input_size = 0;
    uint64_t output_size = 0;
    dxvnpu_status_t status = dxvnpu_get_model_input_size(options.model_path.c_str(), &input_size);
    if (status != DXVNPU_OK) {
        GST_ERROR("dxvnpu_get_model_input_size failed: %s",
                  dxvnpu_status_string(status));
        return false;
    }

    status = dxvnpu_get_model_output_size(options.model_path.c_str(), &output_size);
    if (status != DXVNPU_OK) {
        GST_ERROR("dxvnpu_get_model_output_size failed: %s",
                  dxvnpu_status_string(status));
        return false;
    }

    dxvnpu_config_t cfg = nullptr;
    status = dxvnpu_inference_config_create(&cfg);
    if (status == DXVNPU_OK) {
        status = dxvnpu_inference_config_set_model_path(cfg, options.model_path.c_str());
    }
    if (status == DXVNPU_OK) {
        status = dxvnpu_inference_config_set_use_ort(cfg, options.use_ort ? 1 : 0);
    }
    if (status != DXVNPU_OK) {
        GST_ERROR("failed to configure inference pipeline: %s",
                  dxvnpu_status_string(status));
        if (cfg) {
            dxvnpu_config_destroy(&cfg);
        }
        return false;
    }

    if (options.device_id >= 0) {
        auto ctx = std::make_unique<DeviceContext>();
        ctx->device_id = options.device_id;
        status = dxvnpu_pipeline_create(&ctx->pipeline, cfg,
                                        DXVNPU_PIPELINE_FLAG_NONE, options.device_id);
        if (status != DXVNPU_OK) {
            GST_ERROR("failed to create inference pipeline on device %d: %s",
                      options.device_id, dxvnpu_status_string(status));
            if (cfg) {
                dxvnpu_config_destroy(&cfg);
            }
            return false;
        }
        status = dxvnpu_pipeline_get_device_id(ctx->pipeline, &ctx->device_id);
        if (status != DXVNPU_OK) {
            GST_ERROR("failed to get inference pipeline device ID: %s",
                      dxvnpu_status_string(status));
            if (cfg) {
                dxvnpu_config_destroy(&cfg);
            }
            return false;
        }
        devices_.push_back(std::move(ctx));
    } else {
        uint64_t count = 0;
        status = dxvnpu_get_device_count(&count);
        if (status != DXVNPU_OK || count == 0) {
            GST_ERROR("no VNPU devices found");
            if (cfg) {
                dxvnpu_config_destroy(&cfg);
            }
            return false;
        }

        GST_DEBUG("found %" G_GUINT64_FORMAT " VNPU device(s), initializing...", count);

        for (uint64_t i = 0; i < count; i++) {
            auto ctx = std::make_unique<DeviceContext>();
            ctx->device_id = static_cast<int>(i);
            status = dxvnpu_pipeline_create(&ctx->pipeline, cfg,
                                            DXVNPU_PIPELINE_FLAG_NONE, ctx->device_id);
            if (status != DXVNPU_OK) {
                GST_WARNING("failed to create inference pipeline on device %" G_GUINT64_FORMAT ": %s",
                            i, dxvnpu_status_string(status));
                continue;
            }
            status = dxvnpu_pipeline_get_device_id(ctx->pipeline, &ctx->device_id);
            if (status != DXVNPU_OK) {
                GST_WARNING("failed to get inference pipeline device ID: %s",
                            dxvnpu_status_string(status));
                continue;
            }
            devices_.push_back(std::move(ctx));
        }

        if (devices_.empty()) {
            GST_ERROR("no VNPU devices could be initialized");
            if (cfg) {
                dxvnpu_config_destroy(&cfg);
            }
            return false;
        }
    }
    if (cfg) {
        dxvnpu_config_destroy(&cfg);
    }

    device_count_ = devices_.size();
    input_size_ = static_cast<size_t>(input_size);
    output_size_ = static_cast<size_t>(output_size);

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

    dxvnpu_buffer_t input_buf = nullptr;
    dxvnpu_status_t status = dxvnpu_pipeline_acquire_input_buffer(
        dev->pipeline, &input_buf, DRAIN_TIMEOUT_MS);
    if (status == DXVNPU_OK) {
        status = dxvnpu_buffer_set_data(input_buf, input_ptr, input_size_);
    }
    if (status == DXVNPU_OK) {
        status = dxvnpu_pipeline_put_buffer(dev->pipeline, input_buf,
                                            DRAIN_TIMEOUT_MS);
    }
    if (input_buf) {
        dxvnpu_buffer_release(&input_buf);
    }
    if (status != DXVNPU_OK) {
        GST_ERROR("dxvnpu_pipeline_put_buffer failed on device %d: %s",
                  dev->device_id, dxvnpu_status_string(status));
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
    }

    auto& dev = devices_[req.device_idx];
    const int max_wait = get_max_wait_ms();
    int elapsed = 0;

    while (!flushed_.load()) {
        dxvnpu_buffer_t out = nullptr;
        dxvnpu_status_t status = dxvnpu_pipeline_get_buffer(
            dev->pipeline, &out, DRAIN_TIMEOUT_MS);
        if (status == DXVNPU_OK) {
            size_t seq = get_count_.fetch_add(1);
            GST_TRACE("Got seq=%zu from device[%zu] (device_id=%d, total_put=%zu, total_get=%zu)",
                      seq, req.device_idx, dev->device_id, put_count_.load(), get_count_.load());

            const void* data = nullptr;
            size_t data_size = 0;
            uint32_t tensor_count = 0;
            if (dxvnpu_buffer_view(out, &data, &data_size) != DXVNPU_OK ||
                dxvnpu_buffer_get_tensor_count(out, &tensor_count) != DXVNPU_OK) {
                GST_ERROR("Failed to read inference output buffer on device %d",
                          dev->device_id);
                dxvnpu_buffer_release(&out);
                return false;
            }

            void* target_output = req.output_ptr;
            if (!target_output && data && data_size > 0) {
                output.allocate(data_size);
                target_output = output.data_ptr();
            }

            const size_t copied_size = std::min(data_size, output_size_);
            if (target_output && data && data_size > 0) {
                std::memcpy(target_output, data, copied_size);
            }
            output._tensors.clear();
            for (uint32_t i = 0; i < tensor_count; ++i) {
                dxvnpu_tensor_info_t meta = {};
                if (dxvnpu_buffer_get_tensor_info(out, i, &meta) != DXVNPU_OK) {
                    GST_ERROR("Failed to read inference tensor metadata on device %d",
                              dev->device_id);
                    dxvnpu_buffer_release(&out);
                    return false;
                }
                dxs::DXTensor t;
                t._name = meta.name;
                t._shape.assign(meta.shape, meta.shape + meta.ndims);
                t._type = static_cast<dxs::DataType>(meta.data_type);
                t._elemSize = meta.element_size;
                if (target_output && meta.data_offset <= copied_size &&
                    meta.data_size <= copied_size - meta.data_offset) {
                    t._data = static_cast<uint8_t*>(target_output) + meta.data_offset;
                } else if (target_output) {
                    GST_WARNING("Tensor '%s' exceeds inference output payload",
                                meta.name);
                }
                output._tensors.push_back(t);
            }

            dxvnpu_buffer_release(&out);
            {
                std::lock_guard<std::mutex> lock(mutex_);
                pending_requests_.pop();
            }
            return true;
        }

        if (status == DXVNPU_END_OF_STREAM) {
            GST_WARNING("Unexpected EOS from inference pipeline on device %d", dev->device_id);
            return false;
        }

        if (status != DXVNPU_TIMEOUT) {
            GST_WARNING("dxvnpu_pipeline_get_buffer failed on device %d: %s",
                        dev->device_id, dxvnpu_status_string(status));
            return false;
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
            return false;
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
    size_t drained = 0;
    for (auto& dev : devices_) {
        while (true) {
            dxvnpu_buffer_t out = nullptr;
            dxvnpu_status_t status = dxvnpu_pipeline_get_buffer(
                dev->pipeline, &out, DRAIN_TIMEOUT_MS);
            if (status != DXVNPU_OK) {
                break;
            }
            dxvnpu_buffer_release(&out);
            drained++;
        }
    }
    if (drained > 0) {
        GST_INFO("Reset: drained %zu pending results from inference pipeline(s)", drained);
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
