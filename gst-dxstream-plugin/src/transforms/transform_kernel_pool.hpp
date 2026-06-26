#pragma once

// ---------------------------------------------------------------------------
// TransformKernelPool
//
// Abstracts the difference between dynamic-input kernels (RGA, libyuv) and
// fixed-input kernels (VNPU VideoProcessor).
//
// For dynamic backends (supports_dynamic_input_size == true):
//   A single kernel instance handles all input configs.
//
// For fixed backends (supports_dynamic_input_size == false):
//   One kernel per unique (format, width, height) input config, cached in a
//   hash map for reuse.
//
// Usage:
//   TransformKernelPool pool(dst_template, ops);
//   auto result = pool.transform({VideoFormat::NV12, 1920, 1080}, src, dst);
//   // Primary backend tried first; on failure, libyuv fallback is used.
// ---------------------------------------------------------------------------

#include "video_transform_kernel.hpp"
#include "video_transform_factory.hpp"

#include <gst/gst.h>
#include <memory>
#include <string>
#include <unordered_map>
#include <functional>

namespace dxt {

// ---------------------------------------------------------------------------
// InputConfig — key for the kernel pool
// ---------------------------------------------------------------------------

struct InputConfig {
    VideoFormat format = VideoFormat::NV12;
    int width  = 0;
    int height = 0;

    bool operator==(const InputConfig& o) const {
        return format == o.format && width == o.width && height == o.height;
    }
};

struct InputConfigHash {
    size_t operator()(const InputConfig& c) const {
        size_t h = std::hash<int>()(static_cast<int>(c.format));
        h ^= std::hash<int>()(c.width)  << 16;
        h ^= std::hash<int>()(c.height) << 8;
        return h;
    }
};

// ---------------------------------------------------------------------------
// TransformKernelPool
// ---------------------------------------------------------------------------

class TransformKernelPool {
public:
    TransformKernelPool(const FrameDesc& dst_template, const TransformOps& ops,
                        bool require_dynamic_input = false)
        : dst_template_(dst_template), ops_(ops),
          require_dynamic_input_(require_dynamic_input) {}

    ~TransformKernelPool() = default;

    // Non-copyable, movable
    TransformKernelPool(const TransformKernelPool&) = delete;
    TransformKernelPool& operator=(const TransformKernelPool&) = delete;
    TransformKernelPool(TransformKernelPool&&) = default;
    TransformKernelPool& operator=(TransformKernelPool&&) = default;

    // Get or create a kernel matching the input config.
    // Returns nullptr if no backend can handle the config.
    IVideoTransformKernel* get(const InputConfig& input) {
        if (!determined_) {
            determine_mode(input);
        }

        if (!use_pool_) {
            return single_kernel_.get();
        }

        auto it = pool_.find(input);
        if (it != pool_.end()) {
            return it->second.get();
        }

        // Cache miss — create a new kernel for this input config
        auto kernel = VideoTransformFactory::create(dst_template_, ops_, input.format);
        if (!kernel) {
            return nullptr;
        }
        GST_INFO("TransformKernelPool: new kernel for %dx%d fmt=%d via '%s'",
                 input.width, input.height, static_cast<int>(input.format),
                 kernel->backend_name());
        auto* ptr = kernel.get();
        pool_.emplace(input, std::move(kernel));
        return ptr;
    }

    // Clear all cached kernels (e.g., on stop/finalize)
    void clear() {
        single_kernel_.reset();
        fallback_kernel_.reset();
        pool_.clear();
        determined_ = false;
        use_pool_ = false;
    }

    // Check if pool mode is active
    bool is_pool_mode() const { return use_pool_; }

    // Transform with automatic libyuv fallback on primary kernel failure.
    // Returns the result from whichever kernel succeeded.
    TransformResult transform(const InputConfig& input,
                              const FrameDesc& src, FrameDesc& dst,
                              int slot_id = 0,
                              const DynamicOps* dynamic = nullptr) {
        auto* kernel = get(input);
        if (!kernel) {
            GST_DEBUG("TransformKernelPool: no kernel available for %dx%d fmt=%d",
                      input.width, input.height, static_cast<int>(input.format));
            return TransformResult{};
        }

        auto result = kernel->transform(src, dst, slot_id, dynamic);
        if (result.success) {
            return result;
        }

        // Primary failed — try libyuv fallback (skip if primary IS libyuv)
        if (std::string(kernel->backend_name()) == "libyuv") {
            return result;
        }

        // DMA-buf without CPU data can't fall back to libyuv
        if (src.memory_type == MemoryType::DMA_BUF && !src.luma_data()) {
            GST_DEBUG("TransformKernelPool: primary failed, no CPU data for libyuv fallback");
            return result;
        }

        auto* fb = get_fallback();
        if (!fb) {
            GST_WARNING("TransformKernelPool: libyuv fallback init failed");
            return result;
        }

        GST_LOG("TransformKernelPool: '%s' failed for %dx%d, falling back to libyuv",
                 kernel->backend_name(), input.width, input.height);
        return fb->transform(src, dst, slot_id, dynamic);
    }

private:
    FrameDesc    dst_template_;
    TransformOps ops_;

    std::unique_ptr<IVideoTransformKernel> single_kernel_;
    std::unordered_map<InputConfig,
                       std::unique_ptr<IVideoTransformKernel>,
                       InputConfigHash> pool_;

    bool determined_ = false;
    bool use_pool_   = false;
    bool require_dynamic_input_ = false;

    // Libyuv fallback kernel — lazily created, single instance
    std::unique_ptr<IVideoTransformKernel> fallback_kernel_;

    IVideoTransformKernel* get_fallback() {
        if (!fallback_kernel_) {
            fallback_kernel_ = VideoTransformFactory::create_backend(
                "libyuv", dst_template_, ops_);
        }
        return fallback_kernel_.get();
    }

    void determine_mode(const InputConfig& first_input) {
        auto kernel = VideoTransformFactory::create(dst_template_, ops_, first_input.format);
        if (!kernel) {
            determined_ = true;
            return;
        }

        const auto& caps = kernel->capabilities();

        // If dynamic input is required (e.g., secondary mode preprocessing),
        // reject fixed-input backends and try next
        if (require_dynamic_input_ && !caps.supports_dynamic_input_size) {
            GST_INFO("TransformKernelPool: backend '%s' rejected "
                     "(requires dynamic input support), trying next",
                     caps.name);
            kernel.reset();
            // Retry without the rejected backend — use libyuv directly
            kernel = VideoTransformFactory::create_backend(
                "libyuv", dst_template_, ops_);
            if (!kernel) {
                determined_ = true;
                return;
            }
            const auto& fallback_caps = kernel->capabilities();
            GST_INFO("TransformKernelPool: using fallback backend '%s' (single mode)",
                     fallback_caps.name);
            single_kernel_ = std::move(kernel);
            use_pool_ = false;
            determined_ = true;
            return;
        }

        use_pool_ = !caps.supports_dynamic_input_size;
        determined_ = true;

        GST_INFO("TransformKernelPool: using backend '%s' (%s mode)",
                 caps.name, use_pool_ ? "pool" : "single");

        if (use_pool_) {
            pool_.emplace(first_input, std::move(kernel));
        } else {
            single_kernel_ = std::move(kernel);
        }
    }
};

}  // namespace dxt
