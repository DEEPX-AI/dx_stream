#ifndef DXCOMMON_H
#define DXCOMMON_H

#include <cstdint>
#include <cstring>
#include <memory>
#include <string>
#include <vector>

namespace dxs {

enum DataType {
    NONE_TYPE = 0,
    FLOAT,  ///< 32bit float
    UINT8,  ///< 8bit unsigned integer
    INT8,   ///< 8bit signed integer
    UINT16, ///< 16bit unsigned integer
    INT16,  ///< 16bit signed integer
    INT32,  ///< 32bit signed integer
    INT64,  ///< 64bit signed integer
    UINT32, ///< 32bit unsigned integer
    UINT64, ///< 64bit unsigned integer
    BBOX,   ///< custom structure for bounding boxes from device
    FACE,   ///< custom structure for faces from device
    POSE,   ///< custom structure for poses boxes from device
    MAX_TYPE,
};

struct DeviceBoundingBox_t {
    float x;
    float y;
    float w;
    float h;
    uint8_t grid_y;
    uint8_t grid_x;
    uint8_t box_idx;
    uint8_t layer_idx;
    float score;
    uint32_t label;
    char padding[4];
};

/// @cond
/** \brief face detection data format from device
 * \headerfile "dxrt/dxrt_api.h"
 */
/// @endcond
struct DeviceFace_t {
    float x;
    float y;
    float w;
    float h;
    uint8_t grid_y;
    uint8_t grid_x;
    uint8_t box_idx;
    uint8_t layer_idx;
    float score;
    float kpts[5][2];
};

/// @cond
/** \brief pose estimation data format from device
 * \headerfile "dxrt/dxrt_api.h"
 */
/// @endcond
struct DevicePose_t {
    float x;
    float y;
    float w;
    float h;
    uint8_t grid_y;
    uint8_t grid_x;
    uint8_t box_idx;
    uint8_t layer_idx;
    float score;
    uint32_t label;
    float kpts[17][3];
    char padding[24];
};

struct DXTensor {
    std::string _name;
    std::vector<int64_t> _shape;
    uint64_t _phyAddr = 0;
    void *_data = nullptr;
    uint32_t _elemSize = 0;
    DataType _type = dxs::DataType::NONE_TYPE;

    DXTensor() = default;
    DXTensor(const DXTensor &) = default;
    DXTensor &operator=(const DXTensor &) = default;
    ~DXTensor() = default;
};

struct DXTensors {
    uint32_t _mem_size = 0;
    std::shared_ptr<void> _data;  // RAII: void* → shared_ptr<void>
    std::vector<DXTensor> _tensors;

    void *data_ptr() const { return _data.get(); }

    void allocate(size_t size) {
        _mem_size = static_cast<uint32_t>(size);
        _data = std::shared_ptr<void>(malloc(size), free);
    }

    DXTensors() = default;
    DXTensors(const DXTensors &) = default;
    DXTensors &operator=(const DXTensors &) = default;
    ~DXTensors() = default;
};

} // namespace dxs

#endif /* DXCOMMON_H */
