#include "dxrt_backend.hpp"

#include <gst/gst.h>
#include <sstream>
#include <vector>

#define GST_CAT_DEFAULT inference_backend_cat
GST_DEBUG_CATEGORY_EXTERN(inference_backend_cat);

static bool version_less_than(const std::string& v1, const std::string& v2) {
    std::vector<int> p1, p2;
    std::stringstream ss1(v1);
    std::stringstream ss2(v2);
    std::string part;
    try {
        while (std::getline(ss1, part, '.')) p1.push_back(std::stoi(part));
        while (std::getline(ss2, part, '.')) p2.push_back(std::stoi(part));
    } catch (...) {
        return false;
    }
    while (p1.size() < p2.size()) p1.push_back(0);
    while (p2.size() < p1.size()) p2.push_back(0);
    for (size_t i = 0; i < p1.size(); ++i) {
        if (p1[i] < p2[i]) return true;
        if (p1[i] > p2[i]) return false;
    }
    return false;
}

DxrtBackend::~DxrtBackend() {
    Flush();
}

bool DxrtBackend::Init(const InferBackendOptions& options) {
    if (options.model_path.empty() ||
        !g_file_test(options.model_path.c_str(), G_FILE_TEST_IS_REGULAR)) {
        g_warning("DxrtBackend: model file not found: %s",
                  options.model_path.c_str());
        return false;
    }

    auto infer_option = std::make_shared<dxrt::InferenceOption>();
    infer_option->useORT = options.use_ort;

    try {
        ie_ = std::make_shared<dxrt::InferenceEngine>(
            options.model_path.c_str(), *infer_option);
    } catch (const dxrt::Exception& e) {
        g_warning("DxrtBackend: failed to load model: %s", e.what());
        return false;
    } catch (const std::exception& e) {
        g_warning("DxrtBackend: failed to load model: %s", e.what());
        return false;
    } catch (...) {
        g_warning("DxrtBackend: failed to create InferenceEngine"
                  " (check device/firmware/driver compatibility)");
        return false;
    }

    std::string version = dxrt::Configuration::GetInstance().GetVersion();
    if (version_less_than(version, "3.0.0")) {
        g_warning("DxrtBackend: DXRT version too low (need >= 3.0.0, got %s)",
                  version.c_str());
        return false;
    }

    output_size_ = ie_->GetOutputSize();
    size_t device_count = dxrt::DevicePool::GetInstance().GetDeviceCount();
    max_pending_ = device_count * 5;

    GST_INFO("DxrtBackend: initialized (DXRT %s, %zu devices, max_pending=%zu)",
             version.c_str(), device_count, max_pending_);
    return true;
}

bool DxrtBackend::Put(void* input_ptr, void* output_ptr) {
    std::unique_lock<std::mutex> lock(mutex_);
    cv_.wait(lock, [this] {
        return flushed_ || pending_entries_.size() < max_pending_;
    });
    if (flushed_) return false;

    int req_id = ie_->RunAsync(input_ptr, nullptr, output_ptr);
    pending_entries_.push({req_id, put_count_, std::chrono::steady_clock::now()});
    put_count_++;
    GST_TRACE("Put req_id=%d (pending=%zu, total_put=%zu)", req_id, pending_entries_.size(), put_count_);
    cv_.notify_all();
    return true;
}

bool DxrtBackend::Get(dxs::DXTensors& output) {
    int req_id;
    size_t pending;
    size_t seq;
    {
        std::unique_lock<std::mutex> lock(mutex_);
        cv_.wait(lock, [this] {
            return flushed_ || !pending_entries_.empty();
        });
        if (flushed_) return false;

        auto& front = pending_entries_.front();
        req_id = front.req_id;
        seq = front.seq_num;
        pending_entries_.pop();
        pending = pending_entries_.size();
    }

    auto results = ie_->Wait(req_id);
    convert_tensor(results, output);
    get_count_++;
    GST_TRACE("Got req_id=%d, seq=%zu (pending=%zu, total_put=%zu, total_get=%zu)",
              req_id, seq, pending, put_count_, get_count_);

    cv_.notify_all();
    return true;
}

void DxrtBackend::Flush() {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        flushed_ = true;
        cv_.notify_all();
    }
}

void DxrtBackend::Reset() {
    std::queue<PendingEntry> to_drain;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        std::swap(to_drain, pending_entries_);
    }
    while (!to_drain.empty()) {
        ie_->Wait(to_drain.front().req_id);
        to_drain.pop();
    }
    {
        std::lock_guard<std::mutex> lock(mutex_);
        flushed_ = false;
        put_count_ = 0;
        get_count_ = 0;
    }
}

size_t DxrtBackend::GetOutputBufferSize() const {
    return output_size_;
}

void DxrtBackend::convert_tensor(const dxrt::TensorPtrs& src, dxs::DXTensors& output) {
    for (size_t i = 0; i < src.size(); i++) {
        dxs::DXTensor t;
        t._name = src[i]->name();
        t._shape = src[i]->shape();
        t._type = static_cast<dxs::DataType>(src[i]->type());
        t._data = src[i]->data();
        t._phyAddr = src[i]->phy_addr();
        t._elemSize = src[i]->elem_size();
        output._tensors.push_back(t);
    }
}
