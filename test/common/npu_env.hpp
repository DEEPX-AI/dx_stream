// NPU environment / model path resolver
//
// Models reside in dx_stream/samples/models/*.dxnn.
// Test code resolves the project root via DX_STREAM_ROOT env or auto-search,
// then returns samples/models/<name>.
//
// Environment variables:
//   DX_STREAM_ROOT    project root (auto-detected if unset)
//   DX_NPU_AVAILABLE  set "0" to report NPU unavailable (required NPU tests fail)

#pragma once

#include <gst/gst.h>
#include <glib.h>
#include <string>

namespace dxtest {

inline bool path_exists(const std::string &p) {
    return g_file_test(p.c_str(), G_FILE_TEST_EXISTS);
}

inline std::string dx_stream_root() {
    const char *env = g_getenv("DX_STREAM_ROOT");
    if (env && *env) {
        std::string root = env;
#ifdef _WIN32
        for (char &c : root) if (c == '\\') c = '/';
#endif
        return root;
    }

    // cwd may be test/_bin or similar — walk up to find root.
    // Accept both forward and back slashes as separator (Windows compat).
    char *cwd_c = g_get_current_dir();
    std::string cur = cwd_c;
    g_free(cwd_c);

    // Normalise to forward slash so one code path handles both OSes.
    for (char &c : cur) if (c == '\\') c = '/';

    for (int i = 0; i < 8; i++) {
        bool has_plugin_dir = path_exists(cur + "/gst-dxstream-plugin") &&
                              path_exists(cur + "/dx_stream");
#ifdef _WIN32
        if (has_plugin_dir && path_exists(cur + "/build.bat")) return cur;
#endif
        if (has_plugin_dir && path_exists(cur + "/build.sh")) return cur;
        size_t slash = cur.find_last_of('/');
        if (slash == std::string::npos || slash == 0) break;
        cur = cur.substr(0, slash);
    }
    return "";  // not found
}

inline std::string resolve_model_path(const char *name) {
    std::string root = dx_stream_root();
    if (root.empty()) {
        g_printerr("[RESOURCE] Cannot locate project root for model: %s\n", name);
        return "";
    }
    std::string p = root + "/dx_stream/samples/models/" + name;
    if (path_exists(p)) return p;
    g_printerr("[RESOURCE] Model not found: %s\n", p.c_str());
    return "";
}

inline std::string resolve_config_path(const char *name) {
    std::string root = dx_stream_root();
    if (root.empty()) return "";
    std::string p = root + "/test/resources/configs/" + name;
    return path_exists(p) ? p : "";
}

inline std::string resolve_resource_path(const char *rel) {
    std::string root = dx_stream_root();
    if (root.empty()) {
        g_printerr("[RESOURCE] Cannot locate project root for test resource: %s\n", rel);
        return "";
    }
    std::string p = root + "/test/resources/" + rel;
    if (path_exists(p)) return p;
    g_printerr("[RESOURCE] Test resource not found: %s\n", p.c_str());
    return "";
}

inline std::string resolve_video_path(const char *name) {
    std::string root = dx_stream_root();
    if (root.empty()) {
        g_printerr("[RESOURCE] Cannot locate project root for video: %s\n", name);
        return "";
    }
    std::string p = root + "/dx_stream/samples/videos/" + name;
    if (path_exists(p)) return p;
    g_printerr("[RESOURCE] Video not found: %s\n", p.c_str());
    return "";
}

// ---------------------------------------------------------------------------
// Platform-aware shared-library path resolver for gstdxstream libs
// Linux:   /usr/local/share/gstdxstream/lib/libXXX.so
// Windows: <root>/install/share/gstdxstream/lib/XXX.dll
//
// linux_name: the Linux-style name, e.g. "libpostprocess_ppu.so" or "libdx_msgconvl.so"
// Returns the full path, or empty string if root cannot be determined.
// ---------------------------------------------------------------------------
inline std::string resolve_lib_path(const char *linux_name) {
#ifdef _WIN32
    std::string root = dx_stream_root();
    if (root.empty()) return "";
    std::string name = linux_name;
    if (name.substr(0, 3) == "lib") name = name.substr(3);
    size_t dot = name.rfind('.');
    if (dot != std::string::npos) name = name.substr(0, dot) + ".dll";
    return root + "/install/share/gstdxstream/lib/" + name;
#else
    return std::string("/usr/local/share/gstdxstream/lib/") + linux_name;
#endif
}

inline bool npu_available() {
    const char *env = g_getenv("DX_NPU_AVAILABLE");
    if (env && g_strcmp0(env, "0") == 0) {
        g_printerr("[RESOURCE] NPU disabled by DX_NPU_AVAILABLE=0\n");
        return false;
    }
    return true;  // assume available by default
}

}  // namespace dxtest
