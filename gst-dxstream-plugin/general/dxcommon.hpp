#ifndef DXCOMMON_H
#define DXCOMMON_H

#include <cstdint>
#include <cstring>
#include <memory>
#include <string>
#include <vector>

namespace dxs {

struct SegClsMap {
    std::vector<unsigned char> data;
    int width = 0;
    int height = 0;

    SegClsMap() = default;
    SegClsMap(const SegClsMap &) = default;
    SegClsMap &operator=(const SegClsMap &) = default;
    ~SegClsMap() = default;
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

using Point = Point_<int>;
using Point_f = Point_<float>;

/**
 * @brief RAII-based input buffer for preprocessing output
 * 
 * Manages memory automatically using shared_ptr with custom deleter.
 * Supports shallow copy through shared_ptr reference counting.
 */
struct InputBuffer {
    std::shared_ptr<uint8_t> data;  ///< Managed memory pointer
    size_t size;                     ///< Buffer size in bytes
    std::vector<int64_t> shape;      ///< Optional: tensor shape for debugging
    std::string name;                ///< Optional: buffer identifier

    InputBuffer() : size(0) {}
    
    /**
     * @brief Allocate input buffer with RAII management
     * @param bytes Buffer size in bytes
     * @param shape Optional tensor shape
     * @param name Optional buffer name
     * @return InputBuffer with allocated memory
     */
    static InputBuffer allocate(size_t bytes, 
                               std::vector<int64_t> shape = {}, 
                               std::string name = "") {
        InputBuffer buf;
        buf.data = std::shared_ptr<uint8_t>(
            static_cast<uint8_t*>(malloc(bytes)),
            free  // custom deleter
        );
        buf.size = bytes;
        buf.shape = shape;
        buf.name = name;
        return buf;
    }
    
    uint8_t* get() { return data.get(); }
    const uint8_t* get() const { return data.get(); }
};

using InputBuffers = std::vector<InputBuffer>;

} // namespace dxs

#endif /* DXCOMMON_H */