#pragma once

// Process-wide headless EGL/GLES 3.0 device shared by every GLES transform kernel.
// Works on any GPU whose EGL exposes EGL_EXT_image_dma_buf_import (Mesa v3d/panfrost/freedreno/etnaviv,
// vendor Mali/PowerVR stacks); every capability is probed at runtime so a missing one only disables the backend.

#include "video_transform_kernel.hpp"

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES3/gl3.h>
#include <GLES2/gl2ext.h>

#include <sys/types.h>

#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace dxt {

// How a source frame is sampled by the shader (matches u_src_mode).
enum class GlesSourceMode : int {
    Nv12Planes = 0,  // Y as R8 + UV as GR88
    PackedRgb  = 1,  // 24-bit RGB read as raw bytes of an RGBA8 texture (manual bilinear)
    Rgb        = 2,  // 24-bit RGB texture with hardware filtering
    I420Planes = 3,  // Y/U/V as three R8 textures
    External   = 4,  // driver-converted YUV (samplerExternalOES)
};

struct GlesProgram {
    GLuint id = 0;
    GLint  tile = -1, target = -1, src_mode = -1, src_swap = -1, dst_swap = -1, dst_w = -1;
    GLint  src_size = -1, pad = -1, crop = -1, content = -1;
};

class GlesDevice {
public:
    static std::shared_ptr<GlesDevice> acquire();
    ~GlesDevice();

    GlesDevice(const GlesDevice&) = delete;
    GlesDevice& operator=(const GlesDevice&) = delete;

    // Serialises GL use; the context is made current only for the duration of a call so any
    // streaming thread can use it.
    std::mutex& mutex() { return mu_; }
    bool make_current();
    void release();

    // Imports (or reuses) the source dma-buf and binds it; returns nullptr if the GPU cannot sample it.
    const GlesProgram* bind_source(const FrameDesc& src, GlesSourceMode& mode);

    EGLImageKHR import_rgba(int fd, int w, int h, int pitch);
    void destroy_image(EGLImageKHR img);
    GLuint texture_from_image(EGLImageKHR img);

    // Cached CPU-readable dma-buf for render targets; -1 when no usable heap exists.
    int alloc_dmabuf(size_t size);
    bool dmabuf_targets_ok() const { return dmabuf_targets_ok_; }
    void disable_dmabuf_targets() { dmabuf_targets_ok_ = false; }

    int max_size() const { return max_size_; }
    const std::string& renderer() const { return renderer_; }

private:
    GlesDevice() = default;

    struct Input;
    bool open();
    bool open_display(EGLDisplay dpy, bool owned);
    bool build_program(bool external);
    bool import_source(const FrameDesc& src, Input& in);
    EGLImageKHR import_plane(int fd, uint32_t fourcc, int w, int h, size_t offset, int pitch);
    EGLImageKHR import_yuv(const FrameDesc& src);
    GLuint make_texture(EGLImageKHR img, GLenum target, GLint filter);
    void drop(Input& in);

    std::mutex mu_;
    int drm_fd_ = -1;
    void* gbm_ = nullptr;
    EGLDisplay dpy_ = EGL_NO_DISPLAY;
    bool own_display_ = false;
    EGLContext ctx_ = EGL_NO_CONTEXT;
    EGLDisplay prev_dpy_ = EGL_NO_DISPLAY;
    EGLContext prev_ctx_ = EGL_NO_CONTEXT;
    EGLSurface prev_draw_ = EGL_NO_SURFACE;
    EGLSurface prev_read_ = EGL_NO_SURFACE;
    PFNEGLCREATEIMAGEKHRPROC create_image_ = nullptr;
    PFNEGLDESTROYIMAGEKHRPROC destroy_image_ = nullptr;
    PFNGLEGLIMAGETARGETTEXTURE2DOESPROC image_target_ = nullptr;
    bool has_external_ = false;
    bool prefer_external_ = false;
    GlesProgram programs_[2];  // [0] plane samplers, [1] samplerExternalOES
    int max_size_ = 0;
    std::string renderer_;
    int heap_fd_ = -2;
    bool dmabuf_targets_ok_ = true;

    std::vector<Input*> inputs_;
    std::vector<std::string> failed_layouts_;
    uint64_t tick_ = 0;
};

}  // namespace dxt
