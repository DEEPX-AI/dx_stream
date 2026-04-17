/*
 * metadata_binding.cpp
 *
 * Python bindings for DX Stream metadata types.
 */

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/numpy.h>
#include <gst/gst.h>
#include "dxcommon.hpp"

#include "gst-dxframemeta.hpp"
#include "gst-dxobjectmeta.hpp"
#include "gst-dxusermeta.hpp"

namespace py = pybind11;

// Custom exception for unsupported tensor data types
class UnsupportedTensorDataTypeException : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

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
            throw UnsupportedTensorDataTypeException("Unsupported tensor data type");
    }
}

// Convert a single DXTensor to a numpy array (zero-copy view)
// The base parameter (shared_ptr capsule) ensures the underlying memory
// stays alive as long as the numpy array exists, preventing use-after-free.
py::array get_tensor_as_numpy(const dxs::DXTensor &tensor,
                              const std::shared_ptr<void> &owner) {
    if (!tensor._data || tensor._shape.empty()) {
        return py::array();
    }
    py::dtype dtype = get_numpy_dtype(tensor._type);
    std::vector<py::ssize_t> shape(tensor._shape.begin(), tensor._shape.end());
    // Create a PyCapsule that prevents the shared_ptr from being freed
    auto capsule = py::capsule(new std::shared_ptr<void>(owner),
                               [](void *p) { delete static_cast<std::shared_ptr<void>*>(p); });
    return py::array(dtype, shape, tensor._data, capsule);
}

// Convert std::map<int, dxs::DXTensors> to Python dict {network_id: [numpy_array, ...]}
py::dict convert_tensor_map_to_dict(const std::map<int, dxs::DXTensors> &tensor_map) {
    py::dict result;
    for (const auto &entry : tensor_map) {
        py::list tensor_list;
        for (const auto &tensor : entry.second._tensors) {
            py::dict info;
            info["name"] = tensor._name;
            info["shape"] = tensor._shape;
            info["type"] = static_cast<int>(tensor._type);
            try {
                info["data"] = get_tensor_as_numpy(tensor, entry.second._data);
            } catch (const UnsupportedTensorDataTypeException &) {
                info["data"] = py::none();
            }
            tensor_list.append(info);
        }
        result[py::int_(entry.first)] = tensor_list;
    }
    return result;
}

// Helper function for converting Python integer address to C++ pointer.
// This is required for Python bindings where addresses are passed as integers.
// NOSONAR: cpp:S3630 - reinterpret_cast is necessary for address-to-pointer conversion in FFI
template<typename T>
T* address_to_pointer(size_t address) {
    return reinterpret_cast<T*>(address);  // NOSONAR
}

// Fetch DXFrameMeta from a raw GstBuffer address.
DXFrameMeta *py_dx_get_frame_meta(size_t gst_buffer_address) {
    auto *buffer = address_to_pointer<GstBuffer>(gst_buffer_address);
    if (!buffer) {
        return nullptr;
    }

    GType api_type = dx_frame_meta_api_get_type();
    if (api_type == 0) {
        return nullptr;
    }

    // NOSONAR: cpp:S3630 - GstMeta to DXFrameMeta requires reinterpret_cast
    return reinterpret_cast<DXFrameMeta *>(gst_buffer_get_meta(buffer, api_type));  // NOSONAR
}

// Create new DXFrameMeta and attach to GstBuffer.
DXFrameMeta *py_dx_create_frame_meta(size_t gst_buffer_address) {
    auto *buffer = address_to_pointer<GstBuffer>(gst_buffer_address);
    if (!buffer) {
        return nullptr;
    }
    buffer = dx_create_frame_meta(buffer);
    return dx_get_frame_meta(buffer);
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
    auto *info = address_to_pointer<GstPadProbeInfo>(probe_info_address);
    
    if (!info) return nullptr;

    auto *buffer = GST_PAD_PROBE_INFO_BUFFER(info);
    if (!buffer) return nullptr;

    GST_PAD_PROBE_INFO_DATA(info) = buffer;

    // 3. Get or create metadata
    DXFrameMeta *meta = dx_get_frame_meta(buffer);
    if (!meta) {
        buffer = dx_create_frame_meta(buffer);
        meta = dx_get_frame_meta(buffer);
    }
    
    return meta;
}

// Context Manager helper struct for "with pydxs.writable_buffer(info) as meta:"
struct WritableBufferContext {
    size_t probe_info_address;
    explicit WritableBufferContext(size_t addr) : probe_info_address(addr) {}
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
        - **Segmentation Semantics**: object `seg_data` is an ROI-local binary mask aligned to `box`, while frame `seg_data` is a full-frame semantic class map stored on DXFrameMeta.
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
    // Enums
    // =========================================================================
    py::enum_<DXUserMetaType>(m, "DXUserMetaType", "User metadata type enumeration")
        .value("FRAME", DXUserMetaType::DX_USER_META_FRAME, "Frame-level user metadata")
        .value("OBJECT", DXUserMetaType::DX_USER_META_OBJECT, "Object-level user metadata")
        .export_values();

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
    py::class_<DXObjectMeta>(m, "DXObjectMeta", R"pbdoc(
        Metadata for detected objects.

        Object-level segmentation is exposed through `seg_data`, `seg_width`, `seg_height`,
        and `seg_format`. When present, `seg_data` stores an ROI-local binary mask aligned
        to `box`.
    )pbdoc")
        // Simple read/write attributes
        .def_readwrite("meta_id", &DXObjectMeta::_meta_id, "Unique metadata ID")
        .def_readwrite("track_id", &DXObjectMeta::_track_id, "Tracking ID")
        .def_readwrite("label", &DXObjectMeta::_label, "Object class label")
        .def_readwrite("confidence", &DXObjectMeta::_confidence, "Detection confidence")
        .def_readwrite("face_confidence", &DXObjectMeta::_face_confidence, "Face detection confidence")
        
        // Read/write properties (strings)
        .def_property(
            "label_name",
            [](const DXObjectMeta &meta) {
                return meta._label_name;
            },
            [](DXObjectMeta &meta, const std::string &name) {
                meta._label_name = name;
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
                std::copy(v.begin(), v.begin() + 4, meta._box.begin());
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
                    std::copy(v.begin(), v.begin() + 4, meta._face_box.begin());
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
            [](DXObjectMeta &meta, const std::vector<float> &pts) { meta._face_landmarks = pts; },
            "Face landmarks (flat array: x, y, conf, x, y, conf, ...)")
        .def_property(
            "face_feature",
            [](DXObjectMeta &meta) { return meta._face_feature; },
            [](DXObjectMeta &meta, const std::vector<float> &v) { meta._face_feature = v; },
            "Face feature vector")
        .def_property(
            "seg_data",
            [](DXObjectMeta &meta) {
                return py::bytes(static_cast<const char *>(static_cast<const void *>(meta._seg_data.data())), meta._seg_data.size());
            },
            [](DXObjectMeta &meta, py::bytes payload) {
                std::string buffer = payload;
                meta._seg_data.assign(buffer.begin(), buffer.end());
            },
            "Instance segmentation data stored as an ROI-local binary mask aligned to box (row-major, 0=background, 255=foreground)")
        .def_readwrite("seg_width", &DXObjectMeta::_seg_width, "ROI-local instance segmentation mask width")
        .def_readwrite("seg_height", &DXObjectMeta::_seg_height, "ROI-local instance segmentation mask height")
        .def_property_readonly(
            "seg_format",
            [](const DXObjectMeta &meta) {
                return meta._seg_data.empty() ? std::string("none") : std::string("roi-binary-mask");
            },
            "Segmentation storage format for seg_data")
        
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
                dx_user_meta_set_data(new_meta, (void *)py_obj, sizeof(PyObject *), static_cast<DXUserMetaType>(type_id),
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
                auto meta_list = dx_get_object_user_metas(&self);
                for (auto user_meta : *meta_list) {
                    result.append(user_meta);
                }
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
    py::class_<DXFrameMeta>(m, "DXFrameMeta", R"pbdoc(
        Metadata for video frames.

        Frame-level segmentation is exposed through `seg_data`, `seg_width`, `seg_height`,
        and `seg_format`. When present, `seg_data` stores a full-frame semantic class map.
    )pbdoc")
        // Simple read/write attributes
        .def_readwrite("stream_id", &DXFrameMeta::_stream_id, "Stream identifier")
        .def_readwrite("width", &DXFrameMeta::_width, "Frame width in pixels")
        .def_readwrite("height", &DXFrameMeta::_height, "Frame height in pixels")
        .def_readwrite("frame_rate", &DXFrameMeta::_frame_rate, "Frame rate (fps)")
        
        // Segmentation data (frame-level, semantic seg)
        .def_property(
            "seg_data",
            [](DXFrameMeta &meta) {
                return py::bytes(static_cast<const char *>(static_cast<const void *>(meta._seg_data.data())), meta._seg_data.size());
            },
            [](DXFrameMeta &meta, py::bytes payload) {
                std::string buffer = payload;
                meta._seg_data.assign(buffer.begin(), buffer.end());
            },
            "Semantic segmentation data (row-major, frame-level class map)")
        .def_readwrite("seg_width", &DXFrameMeta::_seg_width, "Segmentation map width")
        .def_readwrite("seg_height", &DXFrameMeta::_seg_height, "Segmentation map height")

        .def_readwrite("label", &DXFrameMeta::_label, "Primary classification label index (-1 if absent)")
        .def_readwrite("label_name", &DXFrameMeta::_label_name, "Primary classification label name")
        .def_readwrite("label_confidence", &DXFrameMeta::_label_confidence, "Primary classification confidence score")

        .def_property_readonly(
            "seg_format",
            [](const DXFrameMeta &meta) {
                return meta._seg_data.empty() ? std::string("none") : std::string("full-frame-class-map");
            },
            "Segmentation storage format for seg_data")
        
        // Read-only properties (string pointers)
        .def_property_readonly(
            "format",
            [](const DXFrameMeta &meta) {
                return meta._format;
            },
            "Video format string (e.g., 'NV12', 'RGB')")
        .def_property_readonly(
            "name",
            [](const DXFrameMeta &meta) {
                return meta._name;
            },
            "Stream name")
        
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
                for (auto obj_meta : meta._object_meta_list) {
                    if (obj_meta) {
                        objects.append(py::cast(obj_meta, py::return_value_policy::reference));
                    }
                }
                return objects;
            },
            "Get list of all attached DXObjectMeta objects")
        
        // Special methods (Python protocols)
        .def("__iter__",
            [](DXFrameMeta &meta) {
                py::list objects;
                for (auto obj_meta : meta._object_meta_list) {
                    if (obj_meta) {
                        objects.append(py::cast(obj_meta, py::return_value_policy::reference));
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
                dx_user_meta_set_data(new_meta, (void *)py_obj, sizeof(PyObject *), static_cast<DXUserMetaType>(type_id),
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
                auto meta_list = dx_get_frame_user_metas(&self);
                for (auto user_meta : *meta_list) {
                    result.append(user_meta);
                }
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
