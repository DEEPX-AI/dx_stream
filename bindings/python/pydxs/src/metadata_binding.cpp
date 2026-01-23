/*
 * metadata_binding.cpp
 *
 * Python bindings for DX Stream metadata types.
 */

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/numpy.h>
#include <gst/gst.h>

#include "gst-dxframemeta.hpp"
#include "gst-dxobjectmeta.hpp"
#include "gst-dxusermeta.hpp"

namespace py = pybind11;

// Python-owned user meta values need manual ref counting hooks.
void python_object_free_cb(void *data) {
    py::gil_scoped_acquire gil;
    PyObject *py_obj = static_cast<PyObject *>(data);
    Py_XDECREF(py_obj);
}

void *python_object_copy_cb(void *data) {
    py::gil_scoped_acquire gil;
    PyObject *py_obj = static_cast<PyObject *>(data);
    Py_XINCREF(py_obj);
    return data;
}

// Helper function to convert DataType to numpy dtype
py::dtype get_numpy_dtype(dxs::DataType type) {
    switch (type) {
        case dxs::DataType::FLOAT:
            return py::dtype::of<float>();
        case dxs::DataType::UINT8:
            return py::dtype::of<uint8_t>();
        case dxs::DataType::INT8:
            return py::dtype::of<int8_t>();
        case dxs::DataType::UINT16:
            return py::dtype::of<uint16_t>();
        case dxs::DataType::INT16:
            return py::dtype::of<int16_t>();
        case dxs::DataType::INT32:
            return py::dtype::of<int32_t>();
        case dxs::DataType::INT64:
            return py::dtype::of<int64_t>();
        case dxs::DataType::UINT32:
            return py::dtype::of<uint32_t>();
        case dxs::DataType::UINT64:
            return py::dtype::of<uint64_t>();
        default:
            throw std::runtime_error("Unsupported tensor data type");
    }
}

// Get tensor as numpy array (zero-copy, writable)
py::array get_tensor_as_numpy(const dxs::DXTensor &tensor) {
    if (tensor._data == nullptr) {
        throw std::runtime_error("Tensor data is null");
    }
    
    if (tensor._shape.empty()) {
        throw std::runtime_error("Tensor shape is empty");
    }
    
    // Convert shape from int64_t to py::ssize_t
    std::vector<py::ssize_t> shape(tensor._shape.begin(), tensor._shape.end());
    
    // Calculate strides (row-major/C-contiguous)
    std::vector<py::ssize_t> strides(shape.size());
    py::ssize_t stride = tensor._elemSize;
    for (int i = shape.size() - 1; i >= 0; --i) {
        strides[i] = stride;
        stride *= shape[i];
    }
    
    // Get numpy dtype
    py::dtype dtype = get_numpy_dtype(tensor._type);
    
    // Create numpy array (no copy, reference to original data)
    // ⚠️ WARNING: Array is WRITABLE - modifying it will affect the pipeline!
    // Use .copy() if you need to modify without affecting the original data.
    py::array result(dtype, shape, strides, tensor._data, py::none());
    
    // Array is writable by default (no read-only flag set)
    // Advanced users can modify tensor data in-place if needed
    
    return result;
}

// Convert DXTensors to Python list of numpy arrays
py::list convert_dxtensors_to_list(const dxs::DXTensors &tensors) {
    py::list result;
    for (const auto &tensor : tensors._tensors) {
        result.append(get_tensor_as_numpy(tensor));
    }
    return result;
}

// Convert std::map<int, dxs::DXTensors> to Python dict {network_id: [tensor1, tensor2, ...]}
py::dict convert_tensor_map_to_dict(const std::map<int, dxs::DXTensors> &tensor_map) {
    py::dict result;
    for (const auto &[network_id, tensors] : tensor_map) {
        result[py::int_(network_id)] = convert_dxtensors_to_list(tensors);
    }
    return result;
}

// Fetch DXFrameMeta from a raw GstBuffer address.
DXFrameMeta *py_dx_get_frame_meta(size_t gst_buffer_address) {
    GstBuffer *buffer = reinterpret_cast<GstBuffer *>(gst_buffer_address);
    if (!buffer) {
        return nullptr;
    }

    GType api_type = dx_frame_meta_api_get_type();
    if (api_type == 0) {
        return nullptr;
    }

    return reinterpret_cast<DXFrameMeta *>(gst_buffer_get_meta(buffer, api_type));
}

// Create new DXFrameMeta and attach to GstBuffer.
DXFrameMeta *py_dx_create_frame_meta(size_t gst_buffer_address) {
    GstBuffer *buffer = reinterpret_cast<GstBuffer *>(gst_buffer_address);
    if (!buffer) {
        return nullptr;
    }
    return dx_create_frame_meta(buffer);
}

// Add DXObjectMeta to DXFrameMeta.
bool py_dx_add_obj_meta_to_frame(DXFrameMeta *frame_meta, DXObjectMeta *obj_meta) {
    if (!frame_meta || !obj_meta) {
        return false;
    }
    return dx_add_obj_meta_to_frame(frame_meta, obj_meta);
}

// Remove DXObjectMeta from DXFrameMeta.
bool py_dx_remove_obj_meta_from_frame(DXFrameMeta *frame_meta, DXObjectMeta *obj_meta) {
    if (!frame_meta || !obj_meta) {
        return false;
    }
    return dx_remove_obj_meta_from_frame(frame_meta, obj_meta);
}

// Ensure buffer is writable and create/get DXFrameMeta.
// This solves the Python refcount issue by handling writability in C++.
DXFrameMeta *py_dx_ensure_writable_and_create_meta(size_t probe_info_address) {
    GstPadProbeInfo *info = reinterpret_cast<GstPadProbeInfo *>(probe_info_address);
    
    if (!info) return nullptr;

    GstBuffer *buffer = GST_PAD_PROBE_INFO_BUFFER(info);
    if (!buffer) return nullptr;

    // 1. Make buffer writable (may create a copy)
    // Performed at C++ level, so Python refcount is not an issue
    buffer = gst_buffer_make_writable(buffer);
    
    // 2. Update the buffer pointer in ProbeInfo
    // This ensures the pipeline uses the new (writable) buffer downstream
    GST_PAD_PROBE_INFO_DATA(info) = buffer;

    // 3. Get or create metadata
    DXFrameMeta *meta = dx_get_frame_meta(buffer);
    if (!meta) {
        meta = dx_create_frame_meta(buffer);
    }
    
    return meta;
}

// Context Manager helper struct for "with pydxs.writable_buffer(info) as meta:"
struct WritableBufferContext {
    size_t probe_info_address;
    WritableBufferContext(size_t addr) : probe_info_address(addr) {}
};

PYBIND11_MODULE(pydxs, m) {
    m.doc() = R"pbdoc(
        pydxs: Python bindings for DX Stream Metadata
        ---------------------------------------------
        
        This module provides comprehensive access to DX Stream metadata structures and utilities
        for manipulating them within GStreamer probes.

        Key Features:
        - **Metadata Access**: Read and write frame metadata (DXFrameMeta) and object metadata (DXObjectMeta).
        - **Metadata Creation**: Create new metadata for frames using `dx_create_frame_meta`.
        - **User Metadata**: Attach arbitrary Python objects to frames or objects as user metadata.
        - **Safe Writability**: Use the `writable_buffer` context manager to safely modify metadata in probes.
        - **Pythonic API**: Support for iteration over objects, property access, and context managers.

        Classes:
        - `DXFrameMeta`: Represents metadata for a video frame.
        - `DXObjectMeta`: Represents metadata for a detected object.
        - `DXUserMeta`: Wrapper for user-defined metadata (Python objects).
        - `writable_buffer`: Context manager for ensuring buffer writability.
    )pbdoc";

    if (!gst_is_initialized()) {
        gst_init(nullptr, nullptr);
    }

    // =========================================================================
    // Context Manager
    // =========================================================================
    py::class_<WritableBufferContext>(m, "writable_buffer",
        "Context manager for safely accessing writable buffers in probes")
        .def(py::init<size_t>(),
             py::arg("probe_info_address"),
             "Initialize with the address of GstPadProbeInfo (use hash(info))")
        .def("__enter__",
            [](WritableBufferContext &self) {
                return py_dx_ensure_writable_and_create_meta(self.probe_info_address);
            },
            py::return_value_policy::reference,
            "Enter context: Ensure buffer is writable and return DXFrameMeta")
        .def("__exit__",
            [](WritableBufferContext &self, py::object exc_type, py::object exc_value, py::object traceback) {
                // No specific cleanup needed, but required for context manager protocol
            },
            "Exit context");

    // =========================================================================
    // Basic value types
    // =========================================================================
    py::class_<dxs::Point_f>(m, "Point_f", "2D point with confidence score (for keypoints, landmarks)")
        .def(py::init<float, float, float>(),
             py::arg("x"), py::arg("y"), py::arg("z") = 0.0f,
             "Create a point with coordinates and confidence (z = confidence score)")
        .def(py::init<>(), "Create a point at origin with zero confidence")
        .def_readwrite("x", &dxs::Point_f::_x, "X coordinate in image")
        .def_readwrite("y", &dxs::Point_f::_y, "Y coordinate in image")
        .def_readwrite("z", &dxs::Point_f::_z, "Confidence score (0.0 - 1.0)");

    py::class_<dxs::SegClsMap>(m, "SegClsMap", "Segmentation classification map")
        .def(py::init<>(), "Create an empty segmentation map")
        .def_readwrite("width", &dxs::SegClsMap::width, "Map width in pixels")
        .def_readwrite("height", &dxs::SegClsMap::height, "Map height in pixels")
        .def_property(
            "data",
            [](dxs::SegClsMap &seg) {
                return py::bytes(reinterpret_cast<const char *>(seg.data.data()), seg.data.size());
            },
            [](dxs::SegClsMap &seg, py::bytes payload) {
                std::string buffer = payload;
                seg.data.assign(buffer.begin(), buffer.end());
            },
            "Raw segmentation bytes (row-major order)");

    py::class_<DXUserMeta>(m, "DXUserMeta", "User-defined metadata wrapper")
        .def(py::init<>(), "Create an empty user metadata object")
        .def_readwrite("type", &DXUserMeta::user_meta_type, "User metadata type ID")
        .def(
            "get_data",
            [](DXUserMeta &self) -> py::object {
                if (self.user_meta_data == nullptr) {
                    return py::none();
                }
                PyObject *py_obj = static_cast<PyObject *>(self.user_meta_data);
                return py::reinterpret_borrow<py::object>(py_obj);
            },
            "Retrieve the stored Python object");

    // =========================================================================
    // Object metadata (DXObjectMeta)
    // =========================================================================
    py::class_<DXObjectMeta>(m, "DXObjectMeta", "Metadata for detected objects")
        // Simple read/write attributes
        .def_readwrite("meta_id", &DXObjectMeta::_meta_id, "Unique metadata ID")
        .def_readwrite("track_id", &DXObjectMeta::_track_id, "Tracking ID")
        .def_readwrite("label", &DXObjectMeta::_label, "Object class label")
        .def_readwrite("confidence", &DXObjectMeta::_confidence, "Detection confidence")
        .def_readwrite("face_confidence", &DXObjectMeta::_face_confidence, "Face detection confidence")
        
        // Read-only properties (computed)
        .def_property_readonly(
            "num_obj_user_meta",
            [](const DXObjectMeta &meta) { return meta._num_obj_user_meta; },
            "Number of attached user metadata")
        
        // Read/write properties (strings)
        .def_property(
            "label_name",
            [](const DXObjectMeta &meta) {
                return meta._label_name ? std::string(meta._label_name->str) : std::string("");
            },
            [](DXObjectMeta &meta, const std::string &name) {
                // Release existing GString if present
                if (meta._label_name) {
                    g_string_free(meta._label_name, TRUE);
                    meta._label_name = nullptr;
                }
                // Create new GString with provided name
                if (!name.empty()) {
                    meta._label_name = g_string_new(name.c_str());
                }
            },
            "Human-readable label name (e.g., 'person', 'car')")
        
        // Read/write properties (arrays)
        .def_property(
            "box",
            [](DXObjectMeta &meta) {
                return std::vector<float>{meta._box[0], meta._box[1], meta._box[2], meta._box[3]};
            },
            [](DXObjectMeta &meta, const std::vector<float> &v) {
                if (v.size() < 4) {
                    throw std::runtime_error("Box must contain 4 floats");
                }
                std::copy(v.begin(), v.begin() + 4, meta._box);
            },
            "Bounding box [left, top, right, bottom] (x1, y1, x2, y2)")
        .def_property(
            "face_box",
            [](DXObjectMeta &meta) {
                return std::vector<float>{meta._face_box[0], meta._face_box[1], meta._face_box[2],
                                          meta._face_box[3]};
            },
            [](DXObjectMeta &meta, const std::vector<float> &v) {
                if (v.size() >= 4) {
                    std::copy(v.begin(), v.begin() + 4, meta._face_box);
                }
            },
            "Face bounding box [left, top, right, bottom] (x1, y1, x2, y2)")
        
        // Read/write properties (vectors)
        .def_property(
            "keypoints",
            [](DXObjectMeta &meta) { return meta._keypoints; },
            [](DXObjectMeta &meta, const std::vector<float> &v) { meta._keypoints = v; },
            "Body keypoints")
        .def_property(
            "body_feature",
            [](DXObjectMeta &meta) { return meta._body_feature; },
            [](DXObjectMeta &meta, const std::vector<float> &v) { meta._body_feature = v; },
            "Body feature vector")
        .def_property(
            "face_landmarks",
            [](DXObjectMeta &meta) { return meta._face_landmarks; },
            [](DXObjectMeta &meta, const std::vector<dxs::Point_f> &pts) { meta._face_landmarks = pts; },
            "Face landmarks")
        .def_property(
            "face_feature",
            [](DXObjectMeta &meta) { return meta._face_feature; },
            [](DXObjectMeta &meta, const std::vector<float> &v) { meta._face_feature = v; },
            "Face feature vector")
        .def_property(
            "segmentation_map",
            [](DXObjectMeta &meta) { return meta._seg_cls_map; },
            [](DXObjectMeta &meta, const dxs::SegClsMap &seg) { meta._seg_cls_map = seg; },
            "Segmentation classification map")
        
        // User metadata methods
        .def(
            "dx_add_user_meta_to_obj",
            [](DXObjectMeta &self, py::object data, int type_id) {
                DXUserMeta *new_meta = dx_acquire_user_meta_from_pool();
                if (!new_meta) {
                    return false;
                }

                PyObject *py_obj = data.ptr();
                Py_XINCREF(py_obj);
                dx_user_meta_set_data(new_meta, (void *)py_obj, sizeof(PyObject *), static_cast<guint>(type_id),
                                      python_object_free_cb, python_object_copy_cb);
                dx_add_user_meta_to_obj(&self, new_meta);
                return true;
            },
            py::arg("data"), py::arg("type_id"),
            "Attach a Python object as user metadata")
        .def(
            "dx_get_object_user_metas",
            [](DXObjectMeta &self) {
                py::list result;
                GList *meta_list = dx_get_object_user_metas(&self);
                for (GList *l = meta_list; l != nullptr; l = l->next) {
                    result.append(static_cast<DXUserMeta *>(l->data));
                }
                g_list_free(meta_list);
                return result;
            },
            "Get list of attached DXUserMeta objects")
        
        // Tensor access methods (Pythonic dict interface)
        .def_property_readonly(
            "input_tensors",
            [](DXObjectMeta &self) -> py::dict {
                return convert_tensor_map_to_dict(self._input_tensors);
            },
            "Get input tensors as dict {network_id: [tensor1, tensor2, ...]} (zero-copy)")
        .def_property_readonly(
            "output_tensors",
            [](DXObjectMeta &self) -> py::dict {
                return convert_tensor_map_to_dict(self._output_tensors);
            },
            "Get output tensors as dict {network_id: [tensor1, tensor2, ...]} (zero-copy)");

    // =========================================================================
    // Frame metadata (DXFrameMeta)
    // =========================================================================
    py::class_<DXFrameMeta>(m, "DXFrameMeta", "Metadata for video frames")
        // Simple read/write attributes
        .def_readwrite("stream_id", &DXFrameMeta::_stream_id, "Stream identifier")
        .def_readwrite("width", &DXFrameMeta::_width, "Frame width in pixels")
        .def_readwrite("height", &DXFrameMeta::_height, "Frame height in pixels")
        .def_readwrite("frame_rate", &DXFrameMeta::_frame_rate, "Frame rate (fps)")
        
        // Read-only properties (string pointers)
        .def_property_readonly(
            "format",
            [](const DXFrameMeta &meta) {
                return meta._format ? std::string(meta._format) : std::string("");
            },
            "Video format string (e.g., 'NV12', 'RGB')")
        .def_property_readonly(
            "name",
            [](const DXFrameMeta &meta) {
                return meta._name ? std::string(meta._name) : std::string("");
            },
            "Stream name")
        .def_property_readonly(
            "num_frame_user_meta",
            [](const DXFrameMeta &meta) { return meta._num_frame_user_meta; },
            "Number of attached user metadata")
        
        // Read/write properties (arrays)
        .def_property(
            "roi",
            [](DXFrameMeta &meta) {
                return std::vector<int>{meta._roi[0], meta._roi[1], meta._roi[2], meta._roi[3]};
            },
            [](DXFrameMeta &meta, const std::vector<int> &v) {
                if (v.size() < 4) {
                    throw std::runtime_error("ROI must contain 4 integers");
                }
                std::copy(v.begin(), v.begin() + 4, meta._roi);
            },
            "Region of interest [left, top, right, bottom] (x1, y1, x2, y2)")
        
        // Computed properties (collections)
        .def_property_readonly(
            "object_meta_list",
            [](DXFrameMeta &meta) {
                py::list objects;
                for (GList *l = meta._object_meta_list; l != nullptr; l = l->next) {
                    if (l->data) {
                        objects.append(py::cast(static_cast<DXObjectMeta *>(l->data),
                                                py::return_value_policy::reference));
                    }
                }
                return objects;
            },
            "Get list of all attached DXObjectMeta objects")
        
        // Special methods (Python protocols)
        .def("__iter__",
            [](DXFrameMeta &meta) {
                py::list objects;
                for (GList *l = meta._object_meta_list; l != nullptr; l = l->next) {
                    if (l->data) {
                        objects.append(py::cast(static_cast<DXObjectMeta *>(l->data),
                                                py::return_value_policy::reference));
                    }
                }
                return py::iter(objects);
            },
            "Iterate over attached DXObjectMeta objects")
        
        // User metadata methods
        .def(
            "dx_add_user_meta_to_frame",
            [](DXFrameMeta &self, py::object data, int type_id) {
                DXUserMeta *new_meta = dx_acquire_user_meta_from_pool();
                if (!new_meta) {
                    return false;
                }

                PyObject *py_obj = data.ptr();
                Py_XINCREF(py_obj);
                dx_user_meta_set_data(new_meta, (void *)py_obj, sizeof(PyObject *), static_cast<guint>(type_id),
                                      python_object_free_cb, python_object_copy_cb);
                dx_add_user_meta_to_frame(&self, new_meta);
                return true;
            },
            py::arg("data"), py::arg("type_id"),
            "Attach a Python object as user metadata")
        .def(
            "dx_get_frame_user_metas",
            [](DXFrameMeta &self) {
                py::list result;
                GList *meta_list = dx_get_frame_user_metas(&self);
                for (GList *l = meta_list; l != nullptr; l = l->next) {
                    result.append(static_cast<DXUserMeta *>(l->data));
                }
                g_list_free(meta_list);
                return result;
            },
            "Get list of attached DXUserMeta objects")
        
        // Tensor access methods (Pythonic dict interface)
        .def_property_readonly(
            "input_tensors",
            [](DXFrameMeta &self) -> py::dict {
                return convert_tensor_map_to_dict(self._input_tensors);
            },
            "Get input tensors as dict {network_id: [tensor1, tensor2, ...]} (zero-copy)")
        .def_property_readonly(
            "output_tensors",
            [](DXFrameMeta &self) -> py::dict {
                return convert_tensor_map_to_dict(self._output_tensors);
            },
            "Get output tensors as dict {network_id: [tensor1, tensor2, ...]} (zero-copy)");

    // =========================================================================
    // Module-level functions
    // =========================================================================
    
    // Metadata access and creation
    m.def("dx_get_frame_meta", &py_dx_get_frame_meta,
          py::arg("gst_buffer_address"),
          py::return_value_policy::reference,
          "Get DXFrameMeta from GstBuffer address");

    m.def("dx_create_frame_meta", &py_dx_create_frame_meta,
          py::arg("gst_buffer_address"),
          py::return_value_policy::reference,
          "Create new DXFrameMeta and attach to GstBuffer");

    m.def("dx_ensure_writable_and_create_meta", &py_dx_ensure_writable_and_create_meta,
          py::arg("probe_info_address"),
          py::return_value_policy::reference,
          "Ensure buffer is writable inside probe and return DXFrameMeta");

    // Object metadata management
    m.def("dx_acquire_obj_meta_from_pool", &dx_acquire_obj_meta_from_pool,
          py::return_value_policy::reference,
          "Acquire DXObjectMeta from pool");

    m.def("dx_add_obj_meta_to_frame", &py_dx_add_obj_meta_to_frame,
          py::arg("frame_meta"), py::arg("obj_meta"),
          "Add DXObjectMeta to DXFrameMeta");

    m.def("dx_remove_obj_meta_from_frame", &py_dx_remove_obj_meta_from_frame,
          py::arg("frame_meta"), py::arg("obj_meta"),
          "Remove DXObjectMeta from DXFrameMeta");
}
