#ifndef DXCOMMON_H
#define DXCOMMON_H

#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

namespace dxs {

struct SegClsMap {
    unsigned char *data;
    int width;
    int height;

    SegClsMap() : data(nullptr), width(0), height(0) {}
    ~SegClsMap() {}
};

template <typename _T> struct Point_ {
    _T _x;
    _T _y;
    _T _z;

    bool operator==(const Point_ &a) {
        if (_x == a._x && _y == a._y && _z == a._z) {
            return true;
        } else {
            return false;
        }
    };
    Point_<_T>(_T x, _T y, _T z = 0) {
        this->_x = x;
        this->_y = y;
        this->_z = z;
    };
    Point_<_T>() {
        this->_x = 0;
        this->_y = 0;
        this->_z = 0;
    };
};

typedef Point_<int> Point;
typedef Point_<float> Point_f;

enum DataType {
    NONE_TYPE = 0,
    UINT8,  ///< 8bit unsigned integer
    UINT16, ///< 16it unsigned integer
    UINT32, ///< 32bit unsigned integer
    UINT64, ///< 64bit unsigned integer
    INT8,   ///< 8bit signed integer
    INT16,  ///< 16bit signed integer
    INT32,  ///< 32bit signed integer
    INT64,  ///< 64bit signed integer
    FLOAT,  ///< 32bit float
    BBOX,   ///< custom structure for bounding boxes from device
    FACE,   ///< custom structure for faces from device
    POSE,   ///< custom structure for poses boxes from device
    MAX_TYPE,
};

typedef struct _DeviceBoundingBox {
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
} DeviceBoundingBox_t;

/// @cond
/** \brief face detection data format from device
 * \headerfile "dxrt/dxrt_api.h"
 */
/// @endcond
typedef struct _DeviceFace {
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
} DeviceFace_t;

/// @cond
/** \brief pose estimation data format from device
 * \headerfile "dxrt/dxrt_api.h"
 */
/// @endcond
typedef struct _DevicePose {
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
} DevicePose_t;

typedef struct _DXTensor {
    std::string _name;
    std::vector<int64_t> _shape;
    uint64_t _phyAddr = 0;
    void *_data = nullptr;
    uint32_t _elemSize = 0;
    DataType _type = dxs::DataType::NONE_TYPE;

    _DXTensor()
        : _name(""), _shape(), _phyAddr(0), _data(nullptr), _elemSize(0),
          _type(dxs::DataType::NONE_TYPE) {}

    _DXTensor(const _DXTensor &other)
        : _name(other._name), _shape(other._shape), _phyAddr(other._phyAddr),
          _data(other._data), _elemSize(other._elemSize), _type(other._type) {}

    _DXTensor &operator=(const _DXTensor &other) {
        _name = other._name;
        _shape = other._shape;
        _phyAddr = other._phyAddr;
        _data = other._data;
        _elemSize = other._elemSize;
        _type = other._type;
        return *this;
    }

    ~_DXTensor() {
        if (_data) {
            _data = nullptr;
        }
        _shape.clear();
    }
} DXTensor;

} // namespace dxs

#endif /* DXCOMMON_H */