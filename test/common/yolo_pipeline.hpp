// YOLO model pipeline builder for inference tests
#pragma once
#include "npu_env.hpp"
#include <string>

namespace dxtest {

// ---------------------------------------------------------------------------
// Platform-aware postprocess library resolver
// Linux:   /usr/local/share/gstdxstream/lib/libXXX.so
// Windows: <root>/install/share/gstdxstream/lib/XXX.dll
//          (lib prefix stripped, .so→.dll)
// ---------------------------------------------------------------------------
inline std::string pp_lib_dir() {
#ifdef _WIN32
    std::string root = dx_stream_root();
    if (!root.empty()) {
        std::string d = root + "/install/share/gstdxstream/lib/";
        if (path_exists(d)) return d;
    }
    return "";
#else
    return "/usr/local/share/gstdxstream/lib/";
#endif
}

// Convert "libpostprocess_ppu.so" → "postprocess_ppu.dll" on Windows,
// return the name unchanged on Linux.
inline std::string pp_lib_name(const char *linux_name) {
#ifdef _WIN32
    std::string name = linux_name;
    // Strip leading "lib"
    if (name.substr(0, 3) == "lib") name = name.substr(3);
    // Replace .so extension with .dll
    size_t dot = name.rfind('.');
    if (dot != std::string::npos) name = name.substr(0, dot) + ".dll";
    return name;
#else
    return linux_name;
#endif
}

inline bool pp_lib_exists(const char *linux_name) {
    std::string dir = pp_lib_dir();
    if (dir.empty()) return false;
    return path_exists(dir + pp_lib_name(linux_name));
}

struct YoloModel {
    const char *model_name;
    int input_size;
    const char *pp_lib;  // Linux name: "libpostprocess_XXX.so"
    const char *pp_func;
    int pad_value;
};

inline std::string build_infer_pipeline(const YoloModel &m,
                                        const char *sink = "appsink name=sink sync=false drop=true") {
    std::string model_file = std::string(m.model_name) + ".dxnn";
    std::string model = resolve_model_path(model_file.c_str());
    std::string image = resolve_resource_path("images/test.jpg");
    std::string dir = pp_lib_dir();
    std::string pp = dir + pp_lib_name(m.pp_lib);

    if (model.empty() || image.empty() || !path_exists(pp))
        return "";

    char buf[2048];
    snprintf(buf, sizeof(buf),
        "filesrc location=%s ! jpegdec ! videoconvert "
        "! video/x-raw,format=RGB "
        "! dxpreprocess resize-width=%d resize-height=%d keep-ratio=true pad-value=%d "
        "! dxinfer model-path=%s backend=dxrt "
        "! dxpostprocess library-file-path=%s function-name=%s "
        "! %s",
        image.c_str(),
        m.input_size, m.input_size, m.pad_value,
        model.c_str(), pp.c_str(), m.pp_func, sink);
    return std::string(buf);
}

inline std::string build_infer_pipeline_with_interval(
    const YoloModel &m, int interval,
    const char *sink = "appsink name=sink sync=false drop=true") {
    std::string model_file = std::string(m.model_name) + ".dxnn";
    std::string model = resolve_model_path(model_file.c_str());
    std::string image = resolve_resource_path("images/test.jpg");
    std::string dir = pp_lib_dir();
    std::string pp = dir + pp_lib_name(m.pp_lib);

    if (model.empty() || image.empty() || !path_exists(pp))
        return "";

    char buf[2048];
    snprintf(buf, sizeof(buf),
        "filesrc location=%s ! jpegdec ! videoconvert "
        "! video/x-raw,format=RGB "
        "! dxpreprocess resize-width=%d resize-height=%d keep-ratio=true "
        "pad-value=%d interval=%d "
        "! dxinfer model-path=%s backend=dxrt "
        "! dxpostprocess library-file-path=%s function-name=%s "
        "! %s",
        image.c_str(),
        m.input_size, m.input_size, m.pad_value, interval,
        model.c_str(), pp.c_str(), m.pp_func, sink);
    return std::string(buf);
}

inline std::string build_infer_pipeline_with_roi(
    const YoloModel &m, const char *roi,
    const char *sink = "appsink name=sink sync=false drop=true") {
    std::string model_file = std::string(m.model_name) + ".dxnn";
    std::string model = resolve_model_path(model_file.c_str());
    std::string image = resolve_resource_path("images/test.jpg");
    std::string dir = pp_lib_dir();
    std::string pp = dir + pp_lib_name(m.pp_lib);

    if (model.empty() || image.empty() || !path_exists(pp))
        return "";

    char buf[2048];
    snprintf(buf, sizeof(buf),
        "filesrc location=%s ! jpegdec ! videoconvert "
        "! video/x-raw,format=RGB "
        "! dxpreprocess resize-width=%d resize-height=%d keep-ratio=true "
        "pad-value=%d roi=%s "
        "! dxinfer model-path=%s backend=dxrt "
        "! dxpostprocess library-file-path=%s function-name=%s "
        "! %s",
        image.c_str(),
        m.input_size, m.input_size, m.pad_value, roi,
        model.c_str(), pp.c_str(), m.pp_func, sink);
    return std::string(buf);
}

inline std::string build_multiframe_infer_pipeline(
    const YoloModel &m, int num_buffers, int interval = 0,
    const char *sink = "appsink name=sink sync=false drop=false") {
    std::string model_file = std::string(m.model_name) + ".dxnn";
    std::string model = resolve_model_path(model_file.c_str());
    std::string image = resolve_resource_path("images/test.jpg");
    std::string dir = pp_lib_dir();
    std::string pp = dir + pp_lib_name(m.pp_lib);

    if (model.empty() || image.empty() || !path_exists(pp))
        return "";

    char buf[2048];
    snprintf(buf, sizeof(buf),
        "filesrc location=%s ! jpegdec ! imagefreeze num-buffers=%d "
        "! videoconvert ! video/x-raw,format=RGB "
        "! dxpreprocess resize-width=%d resize-height=%d keep-ratio=true "
        "pad-value=%d interval=%d "
        "! dxinfer model-path=%s backend=dxrt "
        "! dxpostprocess library-file-path=%s function-name=%s "
        "! %s",
        image.c_str(), num_buffers,
        m.input_size, m.input_size, m.pad_value, interval,
        model.c_str(), pp.c_str(), m.pp_func, sink);
    return std::string(buf);
}

// All 9 YOLO models from test_old
static const YoloModel YOLO_MODELS[] = {
    {"YoloV5S_PPU", 640, "libpostprocess_ppu.so",           "YOLOV5S_PPU", 114},
    {"yolo26n",     640, "libpostprocess_yolo26od.so",       "PostProcess",  114},
    {"YoloV5S",     640, "libpostprocess_yolov5s_6.so",      "PostProcess",  0},
    {"YoloV7",      640, "libpostprocess_yolov7.so",          "PostProcess",  0},
    {"YoloV8N",     640, "libpostprocess_yolov8n.so",         "PostProcess",  0},
    {"YoloV9S",     640, "libpostprocess_yolov9s.so",         "PostProcess",  0},
    {"YOLOV11N",    640, "libpostprocess_yolov11.so",         "PostProcess",  0},
    {"yolo26n-pose",640, "libpostprocess_yolo26pose.so",     "PostProcess",  0},
    {"yolov8m_pose",640, "libpostprocess_yolov8m_pose.so",   "PostProcess",  0},
};
static const int YOLO_MODEL_COUNT = sizeof(YOLO_MODELS) / sizeof(YOLO_MODELS[0]);

// Default model for single-model tests
static const YoloModel YOLO_DEFAULT = YOLO_MODELS[0];

// Model for secondary pipeline tests
static const YoloModel YOLO_SECONDARY_PRIMARY = YOLO_MODELS[0];

struct SecondaryModel {
    const char *model_name;
    int input_size;
    const char *pp_lib;
    const char *pp_func;
    int pad_value;
    int preprocess_id;
    int inference_id;
    int target_class_id;
};

static const SecondaryModel CLASSIFY_MODEL = {
    "EfficientNet_Lite0", 224, "libpostprocess_object_class.so",
    "PostProcess", 0, 2, 2, -1
};

static const SecondaryModel FACE_MODEL = {
    "SCRFD500M", 640, "libpostprocess_scrfd500m.so",
    "PostProcess", 114, 3, 3, 0
};

inline bool secondary_model_available(const SecondaryModel &m) {
    std::string model_file = std::string(m.model_name) + ".dxnn";
    std::string model = resolve_model_path(model_file.c_str());
    std::string dir = pp_lib_dir();
    std::string pp = dir + pp_lib_name(m.pp_lib);
    return !model.empty() && path_exists(pp);
}

}  // namespace dxtest
