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

typedef struct _DXNetworkInput {
    uint8_t *_data;
    size_t _element_size;

    _DXNetworkInput() : _data(nullptr), _element_size(0) {}

    _DXNetworkInput(size_t _size, void *_ptr) : _element_size(_size) {
        _data = static_cast<uint8_t *>(_ptr);
    }

    _DXNetworkInput(const _DXNetworkInput &other)
        : _element_size(other._element_size), _data(other._data) {}

    _DXNetworkInput &operator=(const _DXNetworkInput &other) {
        _element_size = other._element_size;
        _data = other._data;
        return *this;
    }

    ~_DXNetworkInput() {
        if (_data) {
            _data = nullptr;
        }
        _element_size = 0;
    }

} DXNetworkInput;
} // namespace dxs

#endif /* DXCOMMON_H */