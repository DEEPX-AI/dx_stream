#pragma once

#include "infer_backend.hpp"

#include <dxvnpu/modules/inference_module.h>
#include <atomic>
#include <chrono>
#include <memory>
#include <mutex>
#include <queue>
#include <vector>

class DxvnpuBackend : public IInferBackend {
public:
    DxvnpuBackend() = default;
    ~DxvnpuBackend() override;

    bool Init(const InferBackendOptions& options) override;
    bool Put(void* input_ptr, void* output_ptr) override;
    bool Get(dxs::DXTensors& output) override;
    void Flush() override;
    void Reset() override;
    size_t GetOutputBufferSize() const override;
    const char* GetName() const override { return "dxvnpu"; }
    bool IsFlushed() const override { return flushed_.load(); }

private:
    struct DeviceContext {
        std::unique_ptr<dxvnpu::InferenceModule> module;
        int device_id = 0;
    };

    struct PendingRequest {
        size_t device_idx;
        void* output_ptr;
        size_t seq_num;
        std::chrono::steady_clock::time_point put_time;
    };

    std::vector<std::unique_ptr<DeviceContext>> devices_;
    std::queue<PendingRequest> pending_requests_;
    std::mutex mutex_;
    size_t device_count_ = 0;
    size_t next_device_ = 0;  // round-robin index for Put
    std::atomic<size_t> put_count_{0};
    std::atomic<size_t> get_count_{0};

    size_t input_size_ = 0;
    size_t output_size_ = 0;
    std::atomic<bool> flushed_{false};

    static constexpr int DRAIN_TIMEOUT_MS = 100;
    static int get_max_wait_ms();
};
