#include "gles_transform_kernel.hpp"
#include "gles_device.hpp"

#include <gst/gst.h>

#include <linux/dma-buf.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>

#include <algorithm>
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <vector>

GST_DEBUG_CATEGORY(gles_transform_cat);
#define GST_CAT_DEFAULT gles_transform_cat

namespace dxt {

namespace {

constexpr int kMaxPerDraw = 32;  // matches the uniform array size in the shaders

void dmabuf_sync(int fd, uint64_t flags) {
    struct dma_buf_sync s = {};
    s.flags = flags;
    while (ioctl(fd, DMA_BUF_IOCTL_SYNC, &s) < 0 && errno == EINTR) {
    }
}

bool valid_crop(const CropRect& c) {
    return c.w > 0 && c.h > 0;
}

}  // namespace

// RGBA8 render target: a cached dma-buf the CPU reads after a cache invalidate, or (when the GPU cannot
// render into an imported dma-buf) a driver texture read back with glReadPixels.
struct GlesTarget {
    int w = 0, h = 0, pitch = 0;
    int fd = -1;
    uint8_t* map = nullptr;
    size_t size = 0;
    EGLImageKHR img = EGL_NO_IMAGE_KHR;
    GLuint tex = 0, fbo = 0;
    std::vector<uint8_t> readback;

    void destroy(GlesDevice& dev) {
        if (fbo)
            glDeleteFramebuffers(1, &fbo);
        if (tex)
            glDeleteTextures(1, &tex);
        dev.destroy_image(img);
        if (map)
            munmap(map, size);
        if (fd >= 0)
            close(fd);
        *this = GlesTarget();
    }
};

GlesTransformKernel::GlesTransformKernel() {
    static gsize once = 0;
    if (g_once_init_enter(&once)) {
        GST_DEBUG_CATEGORY_INIT(gles_transform_cat, "gles_transform", 0, "OpenGL ES video transform kernel");
        g_once_init_leave(&once, 1);
    }
}

GlesTransformKernel::~GlesTransformKernel() {
    if (!dev_ || !target_)
        return;
    std::lock_guard<std::mutex> lk(dev_->mutex());
    if (dev_->make_current()) {
        target_->destroy(*dev_);
        dev_->release();
    }
}

BackendCaps GlesTransformKernel::capabilities() const {
    BackendCaps caps;
    caps.name = "gles";
    caps.hw_accelerated = true;
    caps.supports_dma_buf = true;
    caps.supports_dynamic_input_size = true;
    caps.src_formats = { VideoFormat::NV12, VideoFormat::I420, VideoFormat::RGB, VideoFormat::BGR };
    caps.dst_formats = { VideoFormat::RGB, VideoFormat::BGR };
    return caps;
}

bool GlesTransformKernel::init(const FrameDesc& dst_template, const TransformOps& ops) {
    const char* disable = getenv("DXSTREAM_GLES_DISABLE");
    if (disable && *disable && strcmp(disable, "0") != 0)
        return false;
    if (dst_template.format != VideoFormat::RGB && dst_template.format != VideoFormat::BGR)
        return false;
    if (dst_template.width <= 0 || dst_template.height <= 0)
        return false;
    dev_ = GlesDevice::acquire();
    if (!dev_)
        return false;
    if ((dst_template.width * 3 + 3) / 4 > dev_->max_size() || dst_template.height > dev_->max_size()) {
        dev_.reset();
        return false;
    }
    target_ = std::make_unique<GlesTarget>();
    return TransformKernelBase::init(dst_template, ops);
}

TransformResult GlesTransformKernel::transform(const FrameDesc& src, FrameDesc& dst, int, const DynamicOps* dynamic) {
    TransformResult res;
    // System-memory input is left to libyuv (an upload would cost more than the conversion).
    if (!initialized_ || src.dma_fd < 0)
        return res;
    const CropRect crop = effective_crop(src, dynamic);
    if (!valid_crop(crop))
        return res;
    FrameDesc* d = &dst;
    res.success = run(src, &crop, &d, 1, &res);
    return res;
}

void GlesTransformKernel::transform_batch(const FrameDesc& src, const CropRect* crops, FrameDesc* dsts, int count,
                                          bool* ok, int) {
    for (int i = 0; i < count; ++i)
        ok[i] = false;
    if (!initialized_ || src.dma_fd < 0 || count <= 0)
        return;

    std::vector<CropRect> eff;
    std::vector<FrameDesc*> out;
    std::vector<int> idx;
    for (int i = 0; i < count; ++i) {
        DynamicOps dyn;
        dyn.crop_override = &crops[i];
        const CropRect c = effective_crop(src, &dyn);
        if (!valid_crop(c))
            continue;
        eff.push_back(c);
        out.push_back(&dsts[i]);
        idx.push_back(i);
    }
    if (eff.empty() || !run(src, eff.data(), out.data(), static_cast<int>(eff.size()), nullptr))
        return;
    for (int i : idx)
        ok[i] = true;
}

bool GlesTransformKernel::ensure_target(int w, int h) {
    GlesTarget& t = *target_;
    if (t.fbo && t.w >= w && t.h >= h)
        return true;
    t.destroy(*dev_);
    t.w = w;
    t.h = h;

    if (dev_->dmabuf_targets_ok()) {
        t.pitch = (t.w * 4 + 63) & ~63;
        t.size = (static_cast<size_t>(t.pitch) * t.h + 4095) & ~size_t(4095);
        t.fd = dev_->alloc_dmabuf(t.size);
        if (t.fd >= 0) {
            void* p = mmap(nullptr, t.size, PROT_READ, MAP_SHARED, t.fd, 0);
            t.map = p == MAP_FAILED ? nullptr : static_cast<uint8_t*>(p);
        }
        if (t.map) {
            t.img = dev_->import_rgba(t.fd, t.w, t.h, t.pitch);
            t.tex = dev_->texture_from_image(t.img);
        }
        if (t.tex) {
            glGenFramebuffers(1, &t.fbo);
            glBindFramebuffer(GL_FRAMEBUFFER, t.fbo);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, t.tex, 0);
            if (glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE)
                return true;
        }
        GST_INFO("GLES: dma-buf render targets unavailable, reading back with glReadPixels");
        dev_->disable_dmabuf_targets();
        t.destroy(*dev_);
        t.w = w;
        t.h = h;
    }

    t.pitch = t.w * 4;
    glGenTextures(1, &t.tex);
    glBindTexture(GL_TEXTURE_2D, t.tex);
    glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, t.w, t.h);
    glGenFramebuffers(1, &t.fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, t.fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, t.tex, 0);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        GST_WARNING("GLES: render target %dx%d incomplete", t.w, t.h);
        t.destroy(*dev_);
        return false;
    }
    return true;
}

bool GlesTransformKernel::run(const FrameDesc& src, const CropRect* crops, FrameDesc* const* dsts, int n,
                              TransformResult* first) {
    const int W = dst_template_.width, H = dst_template_.height;
    const int tex_w = (W * 3 + 3) / 4;
    const size_t row_bytes = static_cast<size_t>(W) * 3;

    std::lock_guard<std::mutex> lk(dev_->mutex());
    if (!dev_->make_current())
        return false;
    struct Release {
        GlesDevice* d;
        ~Release() { d->release(); }
    } release{ dev_.get() };

    GlesSourceMode mode;
    const GlesProgram* prog = dev_->bind_source(src, mode);
    if (!prog)
        return false;

    // Tiles are laid out in an atlas no larger than the GPU limit; larger batches take several passes.
    const int max_size = dev_->max_size();
    const int cols = std::max(1, std::min(n, max_size / tex_w));
    const int per_pass = std::min(n, cols * std::max(1, max_size / H));
    if (!ensure_target(cols * tex_w, ((per_pass + cols - 1) / cols) * H))
        return false;
    GlesTarget& t = *target_;

    glBindFramebuffer(GL_FRAMEBUFFER, t.fbo);
    glViewport(0, 0, t.w, t.h);
    glUseProgram(prog->id);
    glBindVertexArray(0);
    glUniform2f(prog->target, static_cast<float>(t.w), static_cast<float>(t.h));
    glUniform1i(prog->src_mode, static_cast<int>(mode));
    // Rgb mode already picked the RGB888/BGR888 fourcc matching src.format, so only PackedRgb needs a swap.
    glUniform1i(prog->src_swap, mode == GlesSourceMode::PackedRgb && src.format == VideoFormat::BGR ? 1 : 0);
    glUniform1i(prog->dst_swap, dst_template_.format == VideoFormat::BGR ? 1 : 0);
    glUniform1i(prog->dst_w, W);
    glUniform2f(prog->src_size, static_cast<float>(src.width), static_cast<float>(src.height));
    const bool pad = ops_.keep_aspect_ratio && ops_.padding.enabled;
    float pad_rgb[3] = { ops_.padding.pad_r / 255.f, ops_.padding.pad_g / 255.f, ops_.padding.pad_b / 255.f };
    // libyuv writes pad_r/g/b as bytes 0/1/2 for RGB and BGR alike; pre-swap because the shader swaps BGR output.
    if (dst_template_.format == VideoFormat::BGR)
        std::swap(pad_rgb[0], pad_rgb[2]);
    glUniform3f(prog->pad, pad ? pad_rgb[0] : 0.f, pad ? pad_rgb[1] : 0.f, pad ? pad_rgb[2] : 0.f);

    for (int base = 0; base < n; base += per_pass) {
        const int m = std::min(per_pass, n - base);
        for (int k0 = 0; k0 < m; k0 += kMaxPerDraw) {
            const int cnt = std::min(kMaxPerDraw, m - k0);
            float tile[4 * kMaxPerDraw], crop[4 * kMaxPerDraw], content[4 * kMaxPerDraw];
            for (int j = 0; j < cnt; ++j) {
                const int slot = k0 + j;
                const CropRect& c = crops[base + slot];
                int dx, dy, cw, ch;
                compute_dst_rect(c.w, c.h, dx, dy, cw, ch);
                const float tl[4] = { float((slot % cols) * tex_w), float((slot / cols) * H), float(tex_w), float(H) };
                const float cr[4] = { float(c.x), float(c.y), float(c.w), float(c.h) };
                const float ct[4] = { float(dx), float(dy), float(std::max(cw, 1)), float(std::max(ch, 1)) };
                memcpy(tile + 4 * j, tl, sizeof(tl));
                memcpy(crop + 4 * j, cr, sizeof(cr));
                memcpy(content + 4 * j, ct, sizeof(ct));
                if (first && base + slot == 0) {
                    first->content_rect.x = dx;
                    first->content_rect.y = dy;
                    first->content_rect.w = cw;
                    first->content_rect.h = ch;
                    first->content_rect.valid = ops_.keep_aspect_ratio;
                }
            }
            glUniform4fv(prog->tile, cnt, tile);
            glUniform4fv(prog->crop, cnt, crop);
            glUniform4fv(prog->content, cnt, content);
            glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, cnt);
        }

        const uint8_t* pixels = nullptr;
        int pitch = t.pitch;
        if (t.map) {
            GLsync fence = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
            const GLenum wait = glClientWaitSync(fence, GL_SYNC_FLUSH_COMMANDS_BIT, 2000000000ull);
            glDeleteSync(fence);
            if (wait == GL_TIMEOUT_EXPIRED || wait == GL_WAIT_FAILED) {
                GST_WARNING("GLES: GPU wait failed");
                return false;
            }
            dmabuf_sync(t.fd, DMA_BUF_SYNC_START | DMA_BUF_SYNC_READ);
            pixels = t.map;
        } else {
            const int rows = (m + cols - 1) / cols;
            const int read_w = std::min(m, cols) * tex_w;
            pitch = read_w * 4;
            t.readback.resize(static_cast<size_t>(pitch) * rows * H);
            glPixelStorei(GL_PACK_ALIGNMENT, 4);
            glReadPixels(0, 0, read_w, rows * H, GL_RGBA, GL_UNSIGNED_BYTE, t.readback.data());
            pixels = t.readback.data();
        }
        for (int j = 0; j < m; ++j) {
            FrameDesc& d = *dsts[base + j];
            const uint8_t* s = pixels + static_cast<size_t>((j / cols) * H) * pitch + (j % cols) * tex_w * 4;
            const size_t dst_stride = static_cast<size_t>(d.planes[0].stride);
            if (dst_stride == row_bytes && static_cast<size_t>(pitch) == row_bytes) {
                memcpy(d.planes[0].data, s, row_bytes * H);
                continue;
            }
            for (int y = 0; y < H; ++y)
                memcpy(d.planes[0].data + y * dst_stride, s + static_cast<size_t>(y) * pitch, row_bytes);
        }
        if (t.map)
            dmabuf_sync(t.fd, DMA_BUF_SYNC_END | DMA_BUF_SYNC_READ);
    }

    const GLenum err = glGetError();
    if (err != GL_NO_ERROR) {
        GST_WARNING("GLES: GL error 0x%x", err);
        return false;
    }
    return true;
}

}  // namespace dxt
