#pragma once

#include "./../general/dxcommon.hpp"
#include <memory>
#include <string>

struct InferBackendOptions {
    std::string model_path;
    bool use_ort = true;
    int device_id = -1;  // -1: use all devices, >=0: specific device
};

class IInferBackend {
public:
    virtual ~IInferBackend() = default;

    // One-time init. Returns false on failure.
    virtual bool Init(const InferBackendOptions& options) = 0;

    // Submit input for async inference (FIFO). Put/Get are always paired.
    // Blocks if internal queue is full (backpressure).
    // Returns false if flushed (caller should drop the buffer).
    virtual bool Put(void* input_ptr, void* output_ptr) = 0;

    // Block until oldest Put() result is ready, fill output tensor metadata.
    // Returns false if flushed (no valid output).
    virtual bool Get(dxs::DXTensors& output) = 0;

    // FLUSH_START: set flushed flag to unblock Put/Get, clear pending queue.
    // After Flush(): Put() returns false, Get() returns false.
    // Device inflight results are NOT drained here (thread-safety).
    virtual void Flush() = 0;

    // Re-initialize internal state for a new session.
    // Drains any inflight device results, resets counters, sets flushed=false.
    // Must be called after push thread has stopped (thread-safety for drain).
    virtual void Reset() = 0;

    // Output buffer size in bytes (for pre-allocation before Put).
    virtual size_t GetOutputBufferSize() const = 0;

    // Backend name for logging ("dxrt", "dxvnpu").
    virtual const char* GetName() const = 0;

    // Check if backend is in flushed state.
    virtual bool IsFlushed() const = 0;
};
