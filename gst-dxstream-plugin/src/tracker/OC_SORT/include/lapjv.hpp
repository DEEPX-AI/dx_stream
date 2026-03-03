#ifndef OC_SORT_CPP_LAPJV_HPP
#define OC_SORT_CPP_LAPJV_HPP
#include "vector"

constexpr float LARGE = 1000000.0f;

#define NEW(x, t, n) \
    if ((x = (t *) malloc(sizeof(t) * (n))) == 0) { return -1; }
#define FREE(x)   \
    if (x != 0) { \
        free(x);  \
        x = 0;    \
    }
#define SWAP_INDICES(a, b)     \
    {                          \
        int_t _temp_index = a; \
        a = b;                 \
        b = _temp_index;       \
    }

#if 0
#include <assert.h>
#define ASSERT(cond) assert(cond)
#define PRINTF(fmt, ...) printf(fmt, ##__VA_ARGS__)
#define PRINT_COST_ARRAY(a, n)               \
    while (1) {                              \
        printf(#a " = [");                   \
        if ((n) > 0) {                       \
            printf("%f", (a)[0]);            \
            for (uint_t j = 1; j < n; j++) { \
                printf(", %f", (a)[j]);      \
            }                                \
        }                                    \
        printf("]\n");                       \
        break;                               \
    }
#define PRINT_INDEX_ARRAY(a, n)              \
    while (1) {                              \
        printf(#a " = [");                   \
        if ((n) > 0) {                       \
            printf("%d", (a)[0]);            \
            for (uint_t j = 1; j < n; j++) { \
                printf(", %d", (a)[j]);      \
            }                                \
        }                                    \
        printf("]\n");                       \
        break;                               \
    }
#else
#define ASSERT(cond)
#define PRINTF(fmt, ...)
#define PRINT_COST_ARRAY(a, n)
#define PRINT_INDEX_ARRAY(a, n)
#endif


using int_t = signed int;
using uint_t = unsigned int;
using cost_t = float;
using boolean = char;
enum class fp_t {
    FP_1 = 1,
    FP_2 = 2,
    FP_DYNAMIC = 3
};

int_t lapjv_internal(
    const uint_t n, cost_t* const* cost,
    int_t* x, int_t* y);

int_t lapmod_internal(
    const uint_t n, cost_t* cc, uint_t* ii, uint_t* kk,
    int_t* x, int_t* y, fp_t fp_version);

float execLapjv(const std::vector<std::vector<float>>& cost, std::vector<int>& rowsol,
    std::vector<int>& colsol, bool extend_cost, float cost_limit, bool return_cost);
#endif//OC_SORT_CPP_LAPJV_HPP