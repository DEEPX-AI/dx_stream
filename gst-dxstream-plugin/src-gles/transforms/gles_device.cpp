#include "gles_device.hpp"

#include <gst/gst.h>

#ifdef HAVE_GBM
#include <gbm.h>
#endif

#include <fcntl.h>
// UAPI headers older than Linux 5.6 (e.g. Ubuntu 20.04) lack dma-heap; render targets then use glReadPixels.
#if __has_include(<linux/dma-heap.h>)
#include <linux/dma-heap.h>
#define DXS_HAVE_DMA_HEAP 1
#endif
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <unistd.h>

#include <algorithm>
#include <cstdlib>
#include <cstring>

GST_DEBUG_CATEGORY_EXTERN(gles_transform_cat);
#define GST_CAT_DEFAULT gles_transform_cat

namespace dxt {

namespace {

constexpr uint32_t fourcc(char a, char b, char c, char d) {
    return uint32_t(uint8_t(a)) | (uint32_t(uint8_t(b)) << 8) | (uint32_t(uint8_t(c)) << 16) |
           (uint32_t(uint8_t(d)) << 24);
}

// DRM fourccs (little-endian component order), defined here to avoid a libdrm build dependency.
constexpr uint32_t kFourccR8       = fourcc('R', '8', ' ', ' ');
constexpr uint32_t kFourccGR88     = fourcc('G', 'R', '8', '8');
constexpr uint32_t kFourccRGB888   = fourcc('R', 'G', '2', '4');  // memory B,G,R
constexpr uint32_t kFourccBGR888   = fourcc('B', 'G', '2', '4');  // memory R,G,B
constexpr uint32_t kFourccABGR8888 = fourcc('A', 'B', '2', '4');  // memory R,G,B,A
constexpr uint32_t kFourccNV12     = fourcc('N', 'V', '1', '2');
constexpr uint32_t kFourccYUV420   = fourcc('Y', 'U', '1', '2');

constexpr size_t kInputCacheSize = 32;
constexpr size_t kFailedLayoutCacheSize = 64;

const char* kVertexShader = R"(#version 300 es
uniform vec4 u_tile[32];
uniform vec2 u_target;
flat out int v_id;
void main() {
    vec2 corner = vec2(float(gl_VertexID & 1), float(gl_VertexID >> 1));
    vec4 t = u_tile[gl_InstanceID];
    gl_Position = vec4((t.xy + corner * t.zw) / u_target * 2.0 - 1.0, 0.0, 1.0);
    v_id = gl_InstanceID;
}
)";

// Each output texel carries 4 consecutive bytes of a packed RGB888 row (GPUs cannot render 24-bit targets).
const char* kFragmentShaderBody = R"(
precision highp float;
precision highp int;
#ifdef EXTERNAL
uniform highp samplerExternalOES u_tex0;
#else
uniform highp sampler2D u_tex0;
uniform highp sampler2D u_tex1;
uniform highp sampler2D u_tex2;
#endif
uniform int u_src_mode;
uniform int u_src_swap;
uniform int u_dst_swap;
uniform int u_dst_w;
uniform vec2 u_src_size;
uniform vec3 u_pad;
uniform vec4 u_tile[32];
uniform vec4 u_crop[32];
uniform vec4 u_content[32];
flat in int v_id;
out vec4 o_color;

#ifndef EXTERNAL
// BT.601 limited range, the matrix libyuv uses for NV12/I420 -> RGB.
vec3 yuv2rgb(float y, float u, float v) {
    float c = (y * 255.0 - 16.0) * 1.164384;
    u = u * 255.0 - 128.0;
    v = v * 255.0 - 128.0;
    return clamp(vec3(c + 1.596027 * v, c - 0.391762 * u - 0.812968 * v, c + 2.017232 * u) * (1.0 / 255.0),
                 0.0, 1.0);
}

vec3 fetch_rgb(ivec2 p) {
    int b = p.x * 3;
    int t = b >> 2;
    int r = b & 3;
    vec4 a = texelFetch(u_tex0, ivec2(t, p.y), 0);
    vec3 v;
    if (r == 0) {
        v = a.xyz;
    } else if (r == 1) {
        v = a.yzw;
    } else {
        vec4 n = texelFetch(u_tex0, ivec2(t + 1, p.y), 0);
        v = (r == 2) ? vec3(a.zw, n.x) : vec3(a.w, n.xy);
    }
    return u_src_swap == 1 ? v.zyx : v;
}

vec3 sample_packed_rgb(vec2 s) {
    vec2 f = s - 0.5;
    vec2 fl = floor(f);
    vec2 w = f - fl;
    ivec2 mx = ivec2(u_src_size) - 1;
    ivec2 i0 = clamp(ivec2(fl), ivec2(0), mx);
    ivec2 i1 = clamp(ivec2(fl) + 1, ivec2(0), mx);
    vec3 a = fetch_rgb(i0);
    vec3 b = fetch_rgb(ivec2(i1.x, i0.y));
    vec3 c = fetch_rgb(ivec2(i0.x, i1.y));
    vec3 d = fetch_rgb(i1);
    return mix(mix(a, b, w.x), mix(c, d, w.x), w.y);
}
#endif

vec3 sample_src(vec2 s) {
    vec2 n = s / u_src_size;
#ifdef EXTERNAL
    return texture(u_tex0, n).rgb;
#else
    if (u_src_mode == 0) {
        vec2 uv = texture(u_tex1, n).rg;
        return yuv2rgb(texture(u_tex0, n).r, uv.x, uv.y);
    }
    if (u_src_mode == 3)
        return yuv2rgb(texture(u_tex0, n).r, texture(u_tex1, n).r, texture(u_tex2, n).r);
    if (u_src_mode == 2)
        return texture(u_tex0, n).rgb;
    return sample_packed_rgb(s);
#endif
}

vec3 shade(int x, int y) {
    vec4 c = u_content[v_id];
    vec2 d = vec2(float(x), float(y)) + 0.5 - c.xy;
    if (d.x < 0.0 || d.y < 0.0 || d.x >= c.z || d.y >= c.w)
        return u_pad;
    vec4 cr = u_crop[v_id];
    return sample_src(clamp(cr.xy + d * (cr.zw / c.zw), cr.xy + 0.5, cr.xy + cr.zw - 0.5));
}

void main() {
    ivec2 lp = ivec2(gl_FragCoord.xy - u_tile[v_id].xy);
    int p0 = (lp.x * 4) / 3;
    int r = lp.x * 4 - p0 * 3;
    vec3 a = shade(p0, lp.y);
    vec3 n = (p0 + 1 < u_dst_w) ? shade(p0 + 1, lp.y) : vec3(0.0);
    if (u_dst_swap == 1) {
        a = a.zyx;
        n = n.zyx;
    }
    if (r == 0)
        o_color = vec4(a, n.x);
    else if (r == 1)
        o_color = vec4(a.yz, n.xy);
    else
        o_color = vec4(a.z, n);
}
)";

bool has_ext(const char* list, const char* name) {
    if (!list)
        return false;
    const size_t len = strlen(name);
    for (const char* p = strstr(list, name); p; p = strstr(p + len, name))
        if ((p == list || p[-1] == ' ') && (p[len] == ' ' || p[len] == '\0'))
            return true;
    return false;
}

bool env_true(const char* name) {
    const char* v = getenv(name);
    return v && strcmp(v, "0") != 0 && *v;
}

GLuint compile(GLenum type, const std::string& src) {
    GLuint s = glCreateShader(type);
    const char* p = src.c_str();
    glShaderSource(s, 1, &p, nullptr);
    glCompileShader(s);
    GLint ok = 0;
    glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[2048] = {};
        glGetShaderInfoLog(s, sizeof(log), nullptr, log);
        GST_WARNING("GLES shader compile failed: %s", log);
        glDeleteShader(s);
        return 0;
    }
    return s;
}

bool is_rgb(VideoFormat f) {
    return f == VideoFormat::RGB || f == VideoFormat::BGR;
}

}  // namespace

struct GlesDevice::Input {
    dev_t dev = 0;
    ino_t ino = 0;
    std::string layout;
    GlesSourceMode mode = GlesSourceMode::Nv12Planes;
    EGLImageKHR img[3] = { EGL_NO_IMAGE_KHR, EGL_NO_IMAGE_KHR, EGL_NO_IMAGE_KHR };
    GLuint tex[3] = { 0, 0, 0 };
    uint64_t used = 0;
};

std::shared_ptr<GlesDevice> GlesDevice::acquire() {
    static std::mutex m;
    static std::weak_ptr<GlesDevice> weak;
    std::lock_guard<std::mutex> lk(m);
    auto dev = weak.lock();
    if (dev)
        return dev;
    dev.reset(new GlesDevice());
    if (!dev->open())
        return nullptr;
    weak = dev;
    return dev;
}

GlesDevice::~GlesDevice() {
    if (dpy_ != EGL_NO_DISPLAY && ctx_ != EGL_NO_CONTEXT) {
        if (make_current()) {
            for (Input* in : inputs_) {
                drop(*in);
                delete in;
            }
            for (auto& p : programs_)
                if (p.id)
                    glDeleteProgram(p.id);
            release();
        }
        eglDestroyContext(dpy_, ctx_);
    }
    // Displays not created by us (default/surfaceless) may be shared with other EGL users in the process.
    if (dpy_ != EGL_NO_DISPLAY && own_display_)
        eglTerminate(dpy_);
#ifdef HAVE_GBM
    if (gbm_)
        gbm_device_destroy(static_cast<gbm_device*>(gbm_));
#endif
    if (drm_fd_ >= 0)
        close(drm_fd_);
    if (heap_fd_ >= 0)
        close(heap_fd_);
}

bool GlesDevice::make_current() {
    prev_dpy_ = eglGetCurrentDisplay();
    prev_ctx_ = eglGetCurrentContext();
    prev_draw_ = eglGetCurrentSurface(EGL_DRAW);
    prev_read_ = eglGetCurrentSurface(EGL_READ);
    return eglMakeCurrent(dpy_, EGL_NO_SURFACE, EGL_NO_SURFACE, ctx_) == EGL_TRUE;
}

// Restores whatever the calling thread had current (normally nothing on GStreamer streaming threads).
void GlesDevice::release() {
    if (prev_dpy_ != EGL_NO_DISPLAY && prev_ctx_ != EGL_NO_CONTEXT && prev_ctx_ != ctx_)
        eglMakeCurrent(prev_dpy_, prev_draw_, prev_read_, prev_ctx_);
    else
        eglMakeCurrent(dpy_, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
}

bool GlesDevice::open() {
    const char* client = eglQueryString(EGL_NO_DISPLAY, EGL_EXTENSIONS);
    auto get_platform_display = reinterpret_cast<PFNEGLGETPLATFORMDISPLAYEXTPROC>(
        eglGetProcAddress("eglGetPlatformDisplayEXT"));
    if (!has_ext(client, "EGL_EXT_platform_base"))
        get_platform_display = nullptr;

#ifdef HAVE_GBM
    // 1. GBM on a DRM render node (headless; DXSTREAM_GLES_DEVICE selects a node explicitly).
    if (get_platform_display && (has_ext(client, "EGL_KHR_platform_gbm") || has_ext(client, "EGL_MESA_platform_gbm"))) {
        std::vector<std::string> nodes;
        if (const char* node = getenv("DXSTREAM_GLES_DEVICE"))
            nodes.push_back(node);
        else
            for (int i = 128; i < 192; ++i)
                nodes.push_back("/dev/dri/renderD" + std::to_string(i));
        for (const auto& node : nodes) {
            int fd = ::open(node.c_str(), O_RDWR | O_CLOEXEC);
            if (fd < 0)
                continue;
            gbm_device* gbm = gbm_create_device(fd);
            EGLDisplay dpy = gbm ? get_platform_display(EGL_PLATFORM_GBM_KHR, gbm, nullptr) : EGL_NO_DISPLAY;
            if (dpy != EGL_NO_DISPLAY && open_display(dpy, true)) {
                drm_fd_ = fd;
                gbm_ = gbm;
                GST_INFO("GLES device: %s on %s", renderer_.c_str(), node.c_str());
                return true;
            }
            if (gbm)
                gbm_device_destroy(gbm);
            close(fd);
        }
    }
#endif
    // 2. Mesa surfaceless platform, 3. the stack's default display (vendor drivers).
    if (get_platform_display && has_ext(client, "EGL_MESA_platform_surfaceless")) {
        EGLDisplay dpy = get_platform_display(EGL_PLATFORM_SURFACELESS_MESA, EGL_DEFAULT_DISPLAY, nullptr);
        if (dpy != EGL_NO_DISPLAY && open_display(dpy, false)) {
            GST_INFO("GLES device: %s (surfaceless)", renderer_.c_str());
            return true;
        }
    }
    EGLDisplay dpy = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (dpy != EGL_NO_DISPLAY && open_display(dpy, false)) {
        GST_INFO("GLES device: %s (default display)", renderer_.c_str());
        return true;
    }
    GST_INFO("GLES backend unavailable: no EGL display with dma-buf import and GLES 3.0");
    return false;
}

bool GlesDevice::open_display(EGLDisplay dpy, bool owned) {
    if (!eglInitialize(dpy, nullptr, nullptr))
        return false;
    auto fail = [&]() {
        if (owned)
            eglTerminate(dpy);
        return false;
    };
    const char* ext = eglQueryString(dpy, EGL_EXTENSIONS);
    if (!has_ext(ext, "EGL_EXT_image_dma_buf_import") || !has_ext(ext, "EGL_KHR_surfaceless_context") ||
        !eglBindAPI(EGL_OPENGL_ES_API))
        return fail();

    EGLConfig config = EGL_NO_CONFIG_KHR;
    if (!has_ext(ext, "EGL_KHR_no_config_context")) {
        const EGLint attrs[] = { EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT_KHR, EGL_NONE };
        EGLint n = 0;
        if (!eglChooseConfig(dpy, attrs, &config, 1, &n) || n < 1)
            return fail();
    }
    const EGLint ctx_attrs[] = { EGL_CONTEXT_CLIENT_VERSION, 3, EGL_NONE };
    EGLContext ctx = eglCreateContext(dpy, config, EGL_NO_CONTEXT, ctx_attrs);
    if (ctx == EGL_NO_CONTEXT)
        return fail();

    dpy_ = dpy;
    ctx_ = ctx;
    own_display_ = owned;
    create_image_ = reinterpret_cast<PFNEGLCREATEIMAGEKHRPROC>(eglGetProcAddress("eglCreateImageKHR"));
    destroy_image_ = reinterpret_cast<PFNEGLDESTROYIMAGEKHRPROC>(eglGetProcAddress("eglDestroyImageKHR"));
    image_target_ =
        reinterpret_cast<PFNGLEGLIMAGETARGETTEXTURE2DOESPROC>(eglGetProcAddress("glEGLImageTargetTexture2DOES"));

    bool ok = create_image_ && destroy_image_ && image_target_ && make_current();
    if (ok) {
        GLint major = 0, rb = 0, tex = 0;
        glGetIntegerv(GL_MAJOR_VERSION, &major);
        const char* gl_ext = reinterpret_cast<const char*>(glGetString(GL_EXTENSIONS));
        ok = major >= 3 && has_ext(gl_ext, "GL_OES_EGL_image");
        has_external_ = has_ext(gl_ext, "GL_OES_EGL_image_external_essl3");
        prefer_external_ = has_external_ && env_true("DXSTREAM_GLES_EXTERNAL_YUV");
        const GLubyte* r = glGetString(GL_RENDERER);
        renderer_ = r ? reinterpret_cast<const char*>(r) : "unknown";
        glGetIntegerv(GL_MAX_RENDERBUFFER_SIZE, &rb);
        glGetIntegerv(GL_MAX_TEXTURE_SIZE, &tex);
        max_size_ = std::min(rb, tex);
        if (ok && build_program(false)) {
            glDisable(GL_DITHER);
            glDisable(GL_BLEND);
            glDisable(GL_DEPTH_TEST);
            glDisable(GL_SCISSOR_TEST);
        } else {
            ok = false;
        }
        release();
    }
    if (!ok) {
        eglDestroyContext(dpy, ctx);
        dpy_ = EGL_NO_DISPLAY;
        ctx_ = EGL_NO_CONTEXT;
        return fail();
    }
    return true;
}

bool GlesDevice::build_program(bool external) {
    GlesProgram& p = programs_[external ? 1 : 0];
    if (p.id)
        return true;
    std::string fs = "#version 300 es\n";
    if (external)
        fs += "#extension GL_OES_EGL_image_external_essl3 : require\n#define EXTERNAL 1\n";
    fs += kFragmentShaderBody;

    GLuint vsh = compile(GL_VERTEX_SHADER, kVertexShader);
    GLuint fsh = compile(GL_FRAGMENT_SHADER, fs);
    if (vsh && fsh) {
        p.id = glCreateProgram();
        glAttachShader(p.id, vsh);
        glAttachShader(p.id, fsh);
        glLinkProgram(p.id);
        GLint linked = 0;
        glGetProgramiv(p.id, GL_LINK_STATUS, &linked);
        if (!linked) {
            char log[2048] = {};
            glGetProgramInfoLog(p.id, sizeof(log), nullptr, log);
            GST_WARNING("GLES program link failed: %s", log);
            glDeleteProgram(p.id);
            p.id = 0;
        }
    }
    if (vsh)
        glDeleteShader(vsh);
    if (fsh)
        glDeleteShader(fsh);
    if (!p.id)
        return false;

    p.tile = glGetUniformLocation(p.id, "u_tile");
    p.target = glGetUniformLocation(p.id, "u_target");
    p.src_mode = glGetUniformLocation(p.id, "u_src_mode");
    p.src_swap = glGetUniformLocation(p.id, "u_src_swap");
    p.dst_swap = glGetUniformLocation(p.id, "u_dst_swap");
    p.dst_w = glGetUniformLocation(p.id, "u_dst_w");
    p.src_size = glGetUniformLocation(p.id, "u_src_size");
    p.pad = glGetUniformLocation(p.id, "u_pad");
    p.crop = glGetUniformLocation(p.id, "u_crop");
    p.content = glGetUniformLocation(p.id, "u_content");
    glUseProgram(p.id);
    glUniform1i(glGetUniformLocation(p.id, "u_tex0"), 0);
    if (!external) {
        glUniform1i(glGetUniformLocation(p.id, "u_tex1"), 1);
        glUniform1i(glGetUniformLocation(p.id, "u_tex2"), 2);
    }
    return true;
}

EGLImageKHR GlesDevice::import_plane(int fd, uint32_t fmt, int w, int h, size_t offset, int pitch) {
    const EGLint attrs[] = { EGL_WIDTH, w,
                             EGL_HEIGHT, h,
                             EGL_LINUX_DRM_FOURCC_EXT, static_cast<EGLint>(fmt),
                             EGL_DMA_BUF_PLANE0_FD_EXT, fd,
                             EGL_DMA_BUF_PLANE0_OFFSET_EXT, static_cast<EGLint>(offset),
                             EGL_DMA_BUF_PLANE0_PITCH_EXT, pitch,
                             EGL_NONE };
    return create_image_(dpy_, EGL_NO_CONTEXT, EGL_LINUX_DMA_BUF_EXT, nullptr, attrs);
}

EGLImageKHR GlesDevice::import_yuv(const FrameDesc& src) {
    const bool nv12 = src.format == VideoFormat::NV12;
    std::vector<EGLint> a = { EGL_WIDTH, src.width,
                              EGL_HEIGHT, src.height,
                              EGL_LINUX_DRM_FOURCC_EXT, static_cast<EGLint>(nv12 ? kFourccNV12 : kFourccYUV420),
                              EGL_YUV_COLOR_SPACE_HINT_EXT, EGL_ITU_REC601_EXT,
                              EGL_SAMPLE_RANGE_HINT_EXT, EGL_YUV_NARROW_RANGE_EXT };
    const EGLint keys[3][3] = {
        { EGL_DMA_BUF_PLANE0_FD_EXT, EGL_DMA_BUF_PLANE0_OFFSET_EXT, EGL_DMA_BUF_PLANE0_PITCH_EXT },
        { EGL_DMA_BUF_PLANE1_FD_EXT, EGL_DMA_BUF_PLANE1_OFFSET_EXT, EGL_DMA_BUF_PLANE1_PITCH_EXT },
        { EGL_DMA_BUF_PLANE2_FD_EXT, EGL_DMA_BUF_PLANE2_OFFSET_EXT, EGL_DMA_BUF_PLANE2_PITCH_EXT },
    };
    for (int i = 0; i < (nv12 ? 2 : 3); ++i) {
        a.insert(a.end(), { keys[i][0], src.dma_fd, keys[i][1], static_cast<EGLint>(src.planes[i].offset), keys[i][2],
                            src.planes[i].stride });
    }
    a.push_back(EGL_NONE);
    return create_image_(dpy_, EGL_NO_CONTEXT, EGL_LINUX_DMA_BUF_EXT, nullptr, a.data());
}

GLuint GlesDevice::make_texture(EGLImageKHR img, GLenum target, GLint filter) {
    if (img == EGL_NO_IMAGE_KHR)
        return 0;
    while (glGetError() != GL_NO_ERROR) {
    }
    GLuint t = 0;
    glGenTextures(1, &t);
    glBindTexture(target, t);
    image_target_(target, img);
    glTexParameteri(target, GL_TEXTURE_MIN_FILTER, filter);
    glTexParameteri(target, GL_TEXTURE_MAG_FILTER, filter);
    glTexParameteri(target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    if (glGetError() != GL_NO_ERROR) {
        glDeleteTextures(1, &t);
        return 0;
    }
    return t;
}

void GlesDevice::drop(Input& in) {
    for (int i = 0; i < 3; ++i) {
        if (in.tex[i])
            glDeleteTextures(1, &in.tex[i]);
        destroy_image(in.img[i]);
        in.tex[i] = 0;
        in.img[i] = EGL_NO_IMAGE_KHR;
    }
}

bool GlesDevice::import_source(const FrameDesc& src, Input& in) {
    const int w = src.width, h = src.height, cw = (w + 1) / 2, ch = (h + 1) / 2;
    const int fd = src.dma_fd;
    const auto& pl = src.planes;
    if (w > max_size_ || h > max_size_)
        return false;

    std::vector<GlesSourceMode> order;
    if (is_rgb(src.format)) {
        order = { GlesSourceMode::Rgb, GlesSourceMode::PackedRgb };
    } else {
        const GlesSourceMode planes =
            src.format == VideoFormat::NV12 ? GlesSourceMode::Nv12Planes : GlesSourceMode::I420Planes;
        if (prefer_external_)
            order = { GlesSourceMode::External, planes };
        else
            order = { planes, GlesSourceMode::External };
    }

    for (GlesSourceMode mode : order) {
        switch (mode) {
            case GlesSourceMode::Nv12Planes:
                in.img[0] = import_plane(fd, kFourccR8, w, h, pl[0].offset, pl[0].stride);
                in.img[1] = import_plane(fd, kFourccGR88, cw, ch, pl[1].offset, pl[1].stride);
                in.tex[0] = make_texture(in.img[0], GL_TEXTURE_2D, GL_LINEAR);
                in.tex[1] = make_texture(in.img[1], GL_TEXTURE_2D, GL_LINEAR);
                if (in.tex[0] && in.tex[1])
                    break;
                drop(in);
                continue;
            case GlesSourceMode::I420Planes:
                if (src.num_planes < 3)
                    continue;
                for (int i = 0; i < 3; ++i) {
                    in.img[i] = import_plane(fd, kFourccR8, i ? cw : w, i ? ch : h, pl[i].offset, pl[i].stride);
                    in.tex[i] = make_texture(in.img[i], GL_TEXTURE_2D, GL_LINEAR);
                }
                if (in.tex[0] && in.tex[1] && in.tex[2])
                    break;
                drop(in);
                continue;
            case GlesSourceMode::Rgb:
                in.img[0] = import_plane(fd, src.format == VideoFormat::RGB ? kFourccBGR888 : kFourccRGB888, w, h,
                                         pl[0].offset, pl[0].stride);
                in.tex[0] = make_texture(in.img[0], GL_TEXTURE_2D, GL_LINEAR);
                if (in.tex[0])
                    break;
                drop(in);
                continue;
            case GlesSourceMode::PackedRgb:
                if ((w * 3) % 4 != 0 || pl[0].stride % 4 != 0 || pl[0].offset % 4 != 0 || w * 3 / 4 > max_size_)
                    continue;
                in.img[0] = import_plane(fd, kFourccABGR8888, w * 3 / 4, h, pl[0].offset, pl[0].stride);
                in.tex[0] = make_texture(in.img[0], GL_TEXTURE_2D, GL_NEAREST);
                if (in.tex[0])
                    break;
                drop(in);
                continue;
            case GlesSourceMode::External:
                if (!has_external_ || !build_program(true))
                    continue;
                in.img[0] = import_yuv(src);
                in.tex[0] = make_texture(in.img[0], GL_TEXTURE_EXTERNAL_OES, GL_LINEAR);
                if (in.tex[0])
                    break;
                drop(in);
                continue;
        }
        in.mode = mode;
        GST_DEBUG("GLES import %dx%d fmt=%d as mode %d", w, h, static_cast<int>(src.format), static_cast<int>(mode));
        return true;
    }
    return false;
}

const GlesProgram* GlesDevice::bind_source(const FrameDesc& src, GlesSourceMode& mode) {
    if (src.dma_fd < 0)
        return nullptr;
    if (!is_rgb(src.format) && src.format != VideoFormat::NV12 && src.format != VideoFormat::I420)
        return nullptr;
    struct stat st;
    if (fstat(src.dma_fd, &st) < 0)
        return nullptr;

    std::string layout = std::to_string(static_cast<int>(src.format)) + ":" + std::to_string(src.width) + "x" +
                         std::to_string(src.height);
    for (int i = 0; i < src.num_planes; ++i)
        layout += ":" + std::to_string(src.planes[i].offset) + "/" + std::to_string(src.planes[i].stride);

    Input* hit = nullptr;
    for (Input* in : inputs_)
        if (in->dev == st.st_dev && in->ino == st.st_ino && in->layout == layout) {
            hit = in;
            break;
        }
    if (!hit) {
        if (std::find(failed_layouts_.begin(), failed_layouts_.end(), layout) != failed_layouts_.end())
            return nullptr;
        if (inputs_.size() >= kInputCacheSize) {
            auto lru = std::min_element(inputs_.begin(), inputs_.end(),
                                        [](const Input* a, const Input* b) { return a->used < b->used; });
            drop(**lru);
            delete *lru;
            inputs_.erase(lru);
        }
        auto* in = new Input();
        in->dev = st.st_dev;
        in->ino = st.st_ino;
        in->layout = layout;
        if (!import_source(src, *in)) {
            GST_INFO("GLES cannot import %s dma-buf layout %s (EGL 0x%x), using fallback",
                     is_rgb(src.format) ? "RGB" : "YUV", layout.c_str(), eglGetError());
            delete in;
            if (failed_layouts_.size() >= kFailedLayoutCacheSize)
                failed_layouts_.erase(failed_layouts_.begin());
            failed_layouts_.push_back(layout);
            return nullptr;
        }
        inputs_.push_back(in);
        hit = in;
    }
    hit->used = ++tick_;
    mode = hit->mode;

    if (mode == GlesSourceMode::External) {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_EXTERNAL_OES, hit->tex[0]);
        return &programs_[1];
    }
    for (int i = 0; i < 3; ++i) {
        glActiveTexture(GL_TEXTURE0 + i);
        glBindTexture(GL_TEXTURE_2D, hit->tex[i]);
    }
    return &programs_[0];
}

EGLImageKHR GlesDevice::import_rgba(int fd, int w, int h, int pitch) {
    return import_plane(fd, kFourccABGR8888, w, h, 0, pitch);
}

void GlesDevice::destroy_image(EGLImageKHR img) {
    if (img != EGL_NO_IMAGE_KHR)
        destroy_image_(dpy_, img);
}

GLuint GlesDevice::texture_from_image(EGLImageKHR img) {
    return make_texture(img, GL_TEXTURE_2D, GL_NEAREST);
}

int GlesDevice::alloc_dmabuf(size_t size) {
#ifndef DXS_HAVE_DMA_HEAP
    (void)size;
    return -1;
#else
    // Heaps are probed once per device; without one the targets fall back to glReadPixels for its lifetime.
    if (heap_fd_ == -2) {
        heap_fd_ = -1;
        // Cached heaps first: the CPU copies the result out after an explicit cache sync.
        for (const char* heap : { "/dev/dma_heap/system", "/dev/dma_heap/linux,cma", "/dev/dma_heap/reserved" }) {
            heap_fd_ = ::open(heap, O_RDWR | O_CLOEXEC);
            if (heap_fd_ >= 0) {
                GST_DEBUG("GLES render targets from %s", heap);
                break;
            }
        }
    }
    if (heap_fd_ < 0)
        return -1;
    struct dma_heap_allocation_data alloc = {};
    alloc.len = size;
    alloc.fd_flags = O_RDWR | O_CLOEXEC;
    if (ioctl(heap_fd_, DMA_HEAP_IOCTL_ALLOC, &alloc) < 0)
        return -1;
    return static_cast<int>(alloc.fd);
#endif
}

}  // namespace dxt
