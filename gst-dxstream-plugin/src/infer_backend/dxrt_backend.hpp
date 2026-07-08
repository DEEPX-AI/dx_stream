#pragma once

#include "infer_backend.hpp"

#include <chrono>
#include <condition_variable>
#include <dxrt/dxrt_api.h>
#include <atomic>
#include <memory>
#include <mutex>
#include <queue>

class DxrtBackend : public IInferBackend {
public:
    DxrtBackend() = default;
    ~DxrtBackend() override;

    bool Init(const InferBackendOptions& options) override;
    bool Put(void* input_ptr, void* output_ptr) override;
    bool Get(dxs::DXTensors& output) override;
    void Flush() override;
    void Reset() override;
    size_t GetOutputBufferSize() const override;
    const char* GetName() const override { return "dxrt"; }
    bool IsFlushed() const override { return flushed_; }

private:
    static void convert_tensor(const dxrt::TensorPtrs& src, dxs::DXTensors& output);

    struct PendingEntry {
        int req_id;
        size_t seq_num;
        std::chrono::steady_clock::time_point put_time;
    };

    std::shared_ptr<dxrt::InferenceEngine> ie_;
    size_t output_size_ = 0;
    size_t max_pending_ = 10;

    std::queue<PendingEntry> pending_entries_;
    std::mutex mutex_;
    std::condition_variable cv_;
    std::atomic<bool> flushed_{false};
    size_t put_count_ = 0;
    size_t get_count_ = 0;
};
