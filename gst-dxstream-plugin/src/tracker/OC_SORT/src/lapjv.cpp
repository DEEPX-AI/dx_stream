#include "../include/lapjv.hpp"
#include <algorithm>
#include <cassert>
#include <cstdio>
#include <cstring>
#include <limits>
#include <stdexcept>
#include <vector>

#ifndef PRINTF
#ifdef DEBUG_LAPJV
#define PRINTF(...) printf(__VA_ARGS__)
#define PRINT_COST_ARRAY(arr, n)                                               \
    do {                                                                       \
        if (n > 0) {                                                           \
            for (uint_t i = 0; i < (n); ++i)                                   \
                PRINTF("%f ", static_cast<double>((arr)[i]));                  \
            PRINTF("\n");                                                      \
        }                                                                      \
    } while (0)
#define PRINT_INDEX_ARRAY(arr, n)                                              \
    do {                                                                       \
        if (n > 0) {                                                           \
            for (uint_t i = 0; i < (n); ++i)                                   \
                PRINTF("%d ", (arr)[i]);                                       \
            PRINTF("\n");                                                      \
        }                                                                      \
    } while (0)
#else
#define PRINTF(...) (void)0
#define PRINT_COST_ARRAY(arr, n) (void)0
#define PRINT_INDEX_ARRAY(arr, n) (void)0
#endif
#endif

void initializeArrays(const uint_t n, std::vector<int_t> &x,
                      std::vector<int_t> &y, std::vector<cost_t> &v) {
    for (uint_t i = 0; i < n; i++) {
        x[i] = -1;
        v[i] = LARGE;
        y[i] = 0;
    }
}

void updateVandY(const uint_t n, cost_t *const *cost, std::vector<cost_t> &v,
                 std::vector<int_t> &y) {
    for (uint_t i = 0; i < n; i++) {
        for (uint_t j = 0; j < n; j++) {
            cost_t c = cost[i][j];
            if (c < v.at(j)) {
                v.at(j) = c;
                y.at(j) = i;
            }
            PRINTF("i=%u, j=%u, c[i,j]=%f, v[j]=%f y[j]=%d\n", i, j,
                   static_cast<double>(c), static_cast<double>(v.at(j)),
                   y.at(j));
        }
    }
}

void assignUniqueMatches(const uint_t n, std::vector<int_t> &x,
                         std::vector<int_t> &y, std::vector<bool> &unique) {
    for (int_t j_signed = static_cast<int_t>(n) - 1; j_signed >= 0;
         --j_signed) {
        auto j = static_cast<uint_t>(j_signed);
        int_t i = y.at(j);
        if (i < 0 || static_cast<uint_t>(i) >= n) {
            PRINTF("Warning: assignUniqueMatches - y[%u] = %d is out of bounds "
                   "for x/unique array of size %u\n",
                   j, i, n);
            continue;
        }
        if (x.at(i) < 0) {
            x.at(i) = j_signed;
        } else {
            unique.at(i) = false;
            y.at(j) = -1;
        }
    }
}

void addFreeRowIfUnassigned(uint_t i, const std::vector<int_t> &x,
                            std::vector<int_t> &free_rows, int_t &n_free_rows) {
    if (x.at(i) < 0) {
        if (static_cast<uint_t>(n_free_rows) < free_rows.size()) {
            free_rows[n_free_rows] = static_cast<int_t>(i);
            n_free_rows++;
        } else {
            PRINTF("Error: addFreeRowIfUnassigned - free_rows is full.\n");
            throw std::overflow_error(
                "free_rows vector is full in addFreeRowIfUnassigned");
        }
    }
}

void adjustPotentialIfUnique(uint_t i, const std::vector<int_t> &x,
                             const std::vector<bool> &unique,
                             std::vector<cost_t> &v, cost_t *const *cost,
                             const uint_t n) {
    if (!unique.at(i))
        return;

    int_t j_signed = x.at(i);
    if (j_signed < 0 || static_cast<uint_t>(j_signed) >= n) {
        PRINTF("Warning: adjustPotentialIfUnique - x[%u] = %d is out of bounds "
               "for v array of size %u\n",
               i, j_signed, n);
        return;
    }
    auto j = static_cast<uint_t>(j_signed);

    cost_t min_val = LARGE;
    for (uint_t j2 = 0; j2 < n; j2++) {
        if (j2 == j)
            continue;
        cost_t c = cost[i][j2] - v.at(j2);
        if (c < min_val) {
            min_val = c;
        }
    }
    PRINTF("v[%u] = %f - %f\n", j, static_cast<double>(v.at(j)),
           static_cast<double>(min_val));
    if (min_val == LARGE) {
        // No adjustment needed when min_val is LARGE
    }
    v.at(j) -= min_val;
}

void adjustValues(const uint_t n, cost_t *const *cost, std::vector<int_t> &x,
                  const std::vector<bool> &unique, std::vector<cost_t> &v,
                  std::vector<int_t> &free_rows, int_t &n_free_rows) {
    for (uint_t i = 0; i < n; i++) {
        if (x.at(i) < 0) {
            addFreeRowIfUnassigned(i, x, free_rows, n_free_rows);
        } else {
            adjustPotentialIfUnique(i, x, unique, v, cost, n);
        }
    }
}

int_t _ccrrt_dense(const uint_t n, cost_t *const *cost,
                   std::vector<int_t> &free_rows, std::vector<int_t> &x,
                   std::vector<int_t> &y, std::vector<cost_t> &v) {
    initializeArrays(n, x, y, v);
    updateVandY(n, cost, v, y);

    PRINT_COST_ARRAY(v.data(), n);
    PRINT_INDEX_ARRAY(y.data(), n);

    std::vector<bool> unique(n, true);

    assignUniqueMatches(n, x, y, unique);

    int_t n_free_rows = 0;
    adjustValues(n, cost, x, unique, v, free_rows, n_free_rows);

    return n_free_rows;
}

struct TwoMinIndicesResult {
    int_t j1;
    int_t j2;
    cost_t v1;
    cost_t v2;
};

TwoMinIndicesResult findTwoMinIndices(const uint_t n, cost_t *const *cost,
                                       int_t free_i_signed,
                                       const std::vector<cost_t> &v) {
    if (n == 0) {
        return {-1, -1, LARGE, LARGE};
    }
    auto free_i = static_cast<uint_t>(free_i_signed);

    int_t j1 = 0;
    cost_t v1 = cost[free_i][0] - v.at(0);
    int_t j2 = -1;
    cost_t v2 = LARGE;

    for (uint_t j = 1; j < n; j++) {
        cost_t c = cost[free_i][j] - v.at(j);
        if (c < v2) {
            if (c >= v1) {
                v2 = c;
                j2 = static_cast<int_t>(j);
            } else {
                v2 = v1;
                v1 = c;
                j2 = j1;
                j1 = static_cast<int_t>(j);
            }
        }
    }
    return {j1, j2, v1, v2};
}

void addToFreeRows(int_t i0_signed, std::vector<int_t> &free_rows,
                   int_t &new_free_rows_count, uint_t n) {
    if (static_cast<uint_t>(new_free_rows_count) < n) {
        free_rows[new_free_rows_count] = i0_signed;
        new_free_rows_count++;
    } else {
        throw std::overflow_error("free_rows is full.");
    }
}

void prependToFreeRows(int_t i0_signed, std::vector<int_t> &free_rows,
                       uint_t &current_read_idx, int_t &new_free_rows_count,
                       uint_t n) {
    if (current_read_idx > 0) {
        --current_read_idx;
        free_rows[current_read_idx] = i0_signed;
    } else {
        PRINTF("Error: current_read_idx cannot be decremented further.\n");
        addToFreeRows(i0_signed, free_rows, new_free_rows_count, n);
    }
}

void updateDualVariable(uint_t j1, cost_t v1_new, std::vector<cost_t> &v) {
    v.at(j1) = v1_new;
}

bool tryAlternativeColumn(int_t i0_signed, int_t j2_signed, uint_t n,
                          const std::vector<int_t> &y, uint_t &j1,
                          int_t &i0_out) {
    if (i0_signed >= 0 && j2_signed >= 0) {
        if (static_cast<uint_t>(j2_signed) < n) {
            j1 = static_cast<uint_t>(j2_signed);
            i0_out = y.at(j1);
            return true;
        } else {
            PRINTF("Warning: j2_signed (%d) is out of bounds.\n", j2_signed);
            i0_out = -1;
            return false;
        }
    }
    return false;
}

struct FastReductionContext {
    uint_t n;
    int_t j2_signed;
    const std::vector<int_t> &y;
    std::vector<cost_t> &v;
};

void processFastReductionPath(uint_t &j1, int_t &i0_signed, bool v1_lowers,
                              cost_t v1_new, const FastReductionContext &ctx) {
    if (v1_lowers) {
        updateDualVariable(j1, v1_new, ctx.v);
    } else {
        tryAlternativeColumn(i0_signed, ctx.j2_signed, ctx.n, ctx.y, j1, i0_signed);
    }
}

void handleFreeRowInFastPath(int_t i0_signed, bool v1_lowers,
                             std::vector<int_t> &free_rows,
                             uint_t &current_read_idx,
                             int_t &new_free_rows_count, uint_t n) {
    if (i0_signed < 0) {
        return;
    }
    
    if (v1_lowers) {
        prependToFreeRows(i0_signed, free_rows, current_read_idx,
                         new_free_rows_count, n);
    } else {
        addToFreeRows(i0_signed, free_rows, new_free_rows_count, n);
    }
}

void processSlowPath(int_t i0_signed, std::vector<int_t> &free_rows,
                     int_t &new_free_rows_count, uint_t n) {
    if (i0_signed >= 0) {
        addToFreeRows(i0_signed, free_rows, new_free_rows_count, n);
    }
}

int_t _carr_dense(const uint_t n, cost_t *const *cost,
                  const int_t n_free_rows_val, std::vector<int_t> &free_rows,
                  std::vector<int_t> &x, std::vector<int_t> &y,
                  std::vector<cost_t> &v) {
    if (n == 0)
        return 0;
    uint_t current_read_idx = 0;
    int_t new_free_rows_count = 0;
    uint_t rr_cnt = 0;
    auto active_free_rows = static_cast<uint_t>(n_free_rows_val);

    while (current_read_idx < active_free_rows) {
        rr_cnt++;
        const int_t free_i = free_rows.at(current_read_idx);
        current_read_idx++;

        TwoMinIndicesResult min_result = findTwoMinIndices(n, cost, free_i, v);
        int_t j1_signed = min_result.j1;
        int_t j2_signed = min_result.j2;
        cost_t v1 = min_result.v1;
        cost_t v2 = min_result.v2;

        if (j1_signed < 0 || static_cast<uint_t>(j1_signed) >= n) {
            PRINTF("Warning: _carr_dense - j1 (%d) is out of bounds for y/v "
                   "array of size %u\n",
                   j1_signed, n);
            x.at(free_i) = -1;
            continue;
        }
        auto j1 = static_cast<uint_t>(j1_signed);

        int_t i0_signed = y.at(j1);
        cost_t v1_new = v.at(j1) - (v2 - v1);
        bool v1_lowers = v1_new < v.at(j1);

        if (rr_cnt < current_read_idx * n || rr_cnt < 2 * n) {
            FastReductionContext fast_ctx = {n, j2_signed, y, v};
            processFastReductionPath(j1, i0_signed, v1_lowers, v1_new, fast_ctx);
            handleFreeRowInFastPath(i0_signed, v1_lowers, free_rows,
                                    current_read_idx, new_free_rows_count, n);
        } else {
            processSlowPath(i0_signed, free_rows, new_free_rows_count, n);
        }
        
        x.at(free_i) = static_cast<int_t>(j1);
        y.at(j1) = free_i;
    }
    return new_free_rows_count;
}

uint_t _find_dense(const uint_t n, uint_t lo, const std::vector<cost_t> &d,
                   std::vector<int_t> &cols) {
    if (n == 0)
        return lo;
    if (lo >= n) {
        PRINTF("Error: _find_dense - lo = %u is out of bounds (n=%u)\n", lo, n);
        return lo;
    }

    if (static_cast<uint_t>(cols.at(lo)) >= d.size()) {
        PRINTF("Error: _find_dense - cols.at(%u) = %d is out of bounds for d "
               "(size %zu)\n",
               lo, cols.at(lo), d.size());
        throw std::out_of_range(
            "cols.at(lo) yields out-of-bounds index for d in _find_dense");
    }
    cost_t mind = d.at(cols.at(lo));
    uint_t current_hi = lo + 1;

    for (uint_t k = current_hi; k < n; ++k) {
        if (static_cast<uint_t>(cols.at(k)) >= d.size()) {
            PRINTF("Error: _find_dense - cols.at(%u) = %d is out of bounds for "
                   "d (size %zu)\n",
                   k, cols.at(k), d.size());
            throw std::out_of_range(
                "cols.at(k) yields out-of-bounds index for d in _find_dense");
        }
        int_t j_col_idx = cols.at(k);
        if (d.at(j_col_idx) <= mind) {
            if (d.at(j_col_idx) < mind) {
                current_hi = lo;
                mind = d.at(j_col_idx);
            }
            if (k != current_hi) {
                cols[k] = cols.at(current_hi);
            }
            if (current_hi < n) {
                cols.at(current_hi) = j_col_idx;
            } else {
                PRINTF("Error: _find_dense - current_hi attempting to write "
                       "out of bounds.\n");
                throw std::invalid_argument(
                    "_find_dense: current_hi out of bounds during write.");
            }
            current_hi++;
        }
    }
    return current_hi;
}

struct ScanRowContext {
    const std::vector<int_t> &cols;
    const std::vector<int_t> &y;
    cost_t *const *cost;
    const std::vector<cost_t> &d;
    const std::vector<cost_t> &v;
};

struct ScanRowResult {
    int_t i_out;
    cost_t mind_out;
    cost_t h_out;
};

int_t prepare_scan_row(uint_t &lo, const ScanRowContext &ctx,
                       ScanRowResult &result) {
    int_t j_signed = ctx.cols.at(lo);
    lo++;
    if (j_signed < 0 || static_cast<uint_t>(j_signed) >= ctx.y.size() ||
        static_cast<uint_t>(j_signed) >= ctx.d.size() ||
        static_cast<uint_t>(j_signed) >= ctx.v.size()) {
        PRINTF("Error: prepare_scan_row - j_signed = %d is out of bounds.\n",
               j_signed);
        throw std::out_of_range(
            "j_signed is out of bounds in prepare_scan_row");
    }
    auto j = static_cast<uint_t>(j_signed);

    int_t i_signed = ctx.y.at(j);
    if (i_signed < 0) {
        PRINTF("Warning: prepare_scan_row - y[%u] is %d (unassigned row), "
               "cost access might be invalid.\n",
               j, i_signed);

        throw std::invalid_argument("prepare_scan_row: y[j] is negative, cannot "
                               "determine row for cost access.");
    }
    auto i = static_cast<uint_t>(i_signed);

    cost_t mind_val = ctx.d.at(j);
    cost_t h_val = ctx.cost[i][j] - ctx.v.at(j) - mind_val;

    PRINTF("i=%u j=%u h=%f\n", i, j, static_cast<double>(h_val));

    result.i_out = i_signed;
    result.mind_out = mind_val;
    result.h_out = h_val;

    return j_signed;
}

struct CandidateContext {
    const uint_t n;
    cost_t *const *cost;
    std::vector<cost_t> &d;
    const std::vector<cost_t> &v;
    std::vector<int_t> &pred;
    const std::vector<int_t> &y;
    std::vector<int_t> &cols;
};

struct ScanDenseContext {
    const uint_t n;
    cost_t *const *cost;
    std::vector<cost_t> &d;
    std::vector<int_t> &cols;
    std::vector<int_t> &pred;
    const std::vector<int_t> &y;
    const std::vector<cost_t> &v;
};

int_t update_candidates(const int_t i_signed, const cost_t mind_val,
                        const cost_t h_val, CandidateContext &ctx,
                        uint_t &hi_ref) {
    if (i_signed < 0) {
        PRINTF("Warning: update_candidates called with invalid row i_signed = "
               "%d\n",
               i_signed);
        return -1;
    }
    auto i = static_cast<uint_t>(i_signed);

    for (uint_t k = hi_ref; k < ctx.n; ++k) {
        int_t j2_signed = ctx.cols.at(k);
        if (j2_signed < 0 || static_cast<uint_t>(j2_signed) >= ctx.d.size() ||
            static_cast<uint_t>(j2_signed) >= ctx.v.size() ||
            static_cast<uint_t>(j2_signed) >= ctx.pred.size() ||
            static_cast<uint_t>(j2_signed) >= ctx.y.size()) {
            PRINTF("Error: update_candidates - j2_signed = %d from cols[%u] is "
                   "out of bounds.\n",
                   j2_signed, k);
            throw std::out_of_range(
                "j2_signed is out of bounds in update_candidates");
        }
        auto j2 = static_cast<uint_t>(j2_signed);
        cost_t cred_ij = ctx.cost[i][j2] - ctx.v.at(j2) - h_val;

        if (cred_ij >= ctx.d.at(j2)) {
            continue;
        }
        
        ctx.d.at(j2) = cred_ij;
        ctx.pred.at(j2) = i_signed;

        if (cred_ij != mind_val) {
            continue;
        }
        
        if (ctx.y.at(j2) < 0) {
            return j2_signed;
        }
        
        if (k != hi_ref) {
            ctx.cols.at(k) = ctx.cols.at(hi_ref);
        }
        
        if (hi_ref >= ctx.n) {
            PRINTF("Error: update_candidates - hi_ref is out of bounds "
                   "during write.\n");
            throw std::invalid_argument("update_candidates: hi_ref out of "
                                   "bounds during write.");
        }
        
        ctx.cols.at(hi_ref) = j2_signed;
        hi_ref++;
    }
    return -1;
}

int_t _scan_dense(ScanDenseContext &ctx, uint_t *plo, uint_t *phi) {
    if (ctx.n == 0) {
        *plo = *phi;
        return -1;
    }
    uint_t lo = *plo;
    uint_t hi = *phi;

    while (lo != hi) {
        if (lo >= ctx.n || hi > ctx.n || lo > hi) {
            PRINTF("Error: _scan_dense - invalid lo/hi values (lo=%u, hi=%u, "
                   "n=%u)\n",
                   lo, hi, ctx.n);
            throw std::invalid_argument("_scan_dense: lo/hi state is invalid.");
        }

        if (ctx.y.at(ctx.cols.at(lo)) < 0) {
            PRINTF("Warning: _scan_dense - trying to scan from a free column "
                   "cols[%u]=%d. This might be unusual.\n",
                   lo, ctx.cols.at(lo));
        }

        ScanRowContext scan_ctx = {ctx.cols, ctx.y, ctx.cost, ctx.d, ctx.v};
        ScanRowResult scan_result;
        prepare_scan_row(lo, scan_ctx, scan_result);

        CandidateContext cand_ctx = {ctx.n, ctx.cost, ctx.d, ctx.v, ctx.pred, ctx.y, ctx.cols};
        int_t result = update_candidates(scan_result.i_out, scan_result.mind_out,
                                         scan_result.h_out, cand_ctx, hi);
        if (result != -1) {
            *plo = lo;
            *phi = hi;
            return result;
        }
    }

    *plo = lo;
    *phi = hi;
    return -1;
}

int_t update_dual_vars_and_check_match(int_t &final_j, uint_t lo, uint_t hi,
                                       const std::vector<int_t> &cols,
                                       const std::vector<int_t> &y,
                                       uint_t n_size) {
    hi = std::min(hi, n_size);
    final_j = -1;

    for (uint_t k = lo; k < hi; k++) {
        int_t j_col_idx = cols.at(k);
        if (j_col_idx < 0 || static_cast<uint_t>(j_col_idx) >= y.size()) {
            PRINTF("Warning: update_dual_vars_and_check_match - cols[%u] = %d "
                   "is out of bounds for y.\n",
                   k, j_col_idx);
            continue;
        }
        if (y.at(j_col_idx) < 0) {
            final_j = j_col_idx;
            return final_j;
        }
    }
    return final_j;
}

void update_dual_costs(std::vector<cost_t> &v, const std::vector<cost_t> &d,
                       const std::vector<int_t> &cols, uint_t n_ready,
                       uint_t lo) {
    if (lo >= cols.size() || (n_ready > 0 && n_ready > cols.size())) {
        PRINTF("Error: update_dual_costs - lo or n_ready is out of bounds.\n");
        throw std::out_of_range("Index out of bounds in update_dual_costs");
    }
    if (lo >= d.size() ||
        (!cols.empty() && (static_cast<uint_t>(cols.at(lo)) >= d.size()))) {
        PRINTF(
            "Error: update_dual_costs - cols[lo] leads to d out of bounds.\n");
        throw std::out_of_range(
            "cols[lo] for d access is out of bounds in update_dual_costs");
    }

    const cost_t mind_val = d.at(cols.at(lo));
    for (uint_t k = 0; k < n_ready; k++) {
        int_t j_col_idx = cols.at(k);
        if (j_col_idx < 0 || static_cast<uint_t>(j_col_idx) >= v.size() ||
            static_cast<uint_t>(j_col_idx) >= d.size()) {
            PRINTF("Warning: update_dual_costs - cols[%u] = %d is out of "
                   "bounds for v/d.\n",
                   k, j_col_idx);
            continue;
        }
        v.at(j_col_idx) += (d.at(j_col_idx) - mind_val);
    }
}

int_t find_path_dense(const uint_t n, cost_t *const *cost,
                      const int_t start_i_signed, const std::vector<int_t> &y,
                      std::vector<cost_t> &v, std::vector<int_t> &pred) {
    if (n == 0)
        return -1;
    if (start_i_signed < 0 || static_cast<uint_t>(start_i_signed) >= n) {
        PRINTF("Error: find_path_dense - start_i_signed = %d is out of bounds "
               "(n=%u).\n",
               start_i_signed, n);
        throw std::out_of_range(
            "start_i_signed is out of bounds in find_path_dense");
    }
    auto start_i = static_cast<uint_t>(start_i_signed);

    std::vector<int_t> cols(n);
    std::vector<cost_t> d(n);

    for (uint_t j_col = 0; j_col < n; j_col++) {
        cols[j_col] = static_cast<int_t>(j_col);
        pred.at(j_col) = start_i_signed;
        d.at(j_col) = cost[start_i][j_col] - v.at(j_col);
    }

    PRINT_COST_ARRAY(d.data(), n);

    uint_t lo = 0;
    uint_t hi = 0;
    uint_t n_ready = 0;
    int_t final_j = -1;

    while (final_j == -1) {
        if (lo == hi) {
            PRINTF("%u..%u -> find\n", lo, hi);
            n_ready = lo;
            hi = _find_dense(n, lo, d, cols);
            PRINTF("check %u..%u\n", lo, hi);
            PRINT_INDEX_ARRAY(cols.data(), n);
            update_dual_vars_and_check_match(final_j, lo, hi, cols, y, n);
        }

        if (final_j == -1) {
            if (lo == hi) {
                PRINTF("Error: find_path_dense - lo == hi after find, but no "
                       "final_j. Path not found.\n");
                throw std::invalid_argument(
                    "Path not found in find_path_dense, Dijkstra stalled.");
            }
            PRINTF("%u..%u -> scan\n", lo, hi);
            ScanDenseContext scan_dense_ctx = {n, cost, d, cols, pred, y, v};
            final_j = _scan_dense(scan_dense_ctx, &lo, &hi);
            PRINT_COST_ARRAY(d.data(), n);
            PRINT_INDEX_ARRAY(cols.data(), n);
            PRINT_INDEX_ARRAY(pred.data(), n);
        }
    }

    PRINTF("found final_j=%d\n", final_j);
    PRINT_INDEX_ARRAY(cols.data(), n);
    update_dual_costs(v, d, cols, n_ready, lo);

    return final_j;
}

int_t _ca_dense(const uint_t n, cost_t *const *cost,
                const int_t n_free_rows_val,
                const std::vector<int_t> &free_rows, std::vector<int_t> &x,
                std::vector<int_t> &y, std::vector<cost_t> &v) {
    if (n == 0 || n_free_rows_val == 0)
        return 0;

    std::vector<int_t> pred(n);
    for (int_t k_free = 0; k_free < n_free_rows_val; ++k_free) {
        int_t current_free_row = free_rows.at(k_free);

        PRINTF("looking at free_i=%d\n", current_free_row);
        int_t j_path_end_col =
            find_path_dense(n, cost, current_free_row, y, v, pred);

        assert(j_path_end_col >= 0 && static_cast<uint_t>(j_path_end_col) < n);
        int_t i_path_row = -1;
        int_t current_j_col = j_path_end_col;
        uint_t path_len = 0;

        while (i_path_row != current_free_row) {
            PRINTF("augment %d\n", current_j_col);
            PRINT_INDEX_ARRAY(pred.data(), n);

            if (current_j_col < 0 || static_cast<uint_t>(current_j_col) >= n) {
                PRINTF("Error: _ca_dense - current_j_col = %d is out of bounds "
                       "(n=%u)\n",
                       current_j_col, n);
                throw std::out_of_range("_ca_dense: current_j_col out of "
                                        "bounds during augmentation.");
            }
            i_path_row = pred.at(current_j_col);
            if (i_path_row < 0 || static_cast<uint_t>(i_path_row) >= n) {
                PRINTF("Error: _ca_dense - i_path_row = %d from pred is out of "
                       "bounds (n=%u)\n",
                       i_path_row, n);
                throw std::out_of_range(
                    "_ca_dense: i_path_row from pred out of bounds.");
            }

            PRINTF("y[%d]=%d -> %d\n", current_j_col, y.at(current_j_col),
                   i_path_row);
            y.at(current_j_col) = i_path_row;

            PRINT_INDEX_ARRAY(x.data(), n);
            int_t old_col_in_x = x.at(i_path_row);
            x.at(i_path_row) = current_j_col;
            current_j_col = old_col_in_x;

            path_len++;
            if (path_len > n) {
                PRINTF(
                    "Error: _ca_dense - Path too long during augmentation.\n");
                assert(false);
                throw std::domain_error(
                    "_ca_dense: Augmenting path is too long.");
            }
        }
    }
    return 0;
}

int lapjv_internal(const uint_t n, cost_t *const *cost, int_t *x_data,
                   int_t *y_data) {
    if (n == 0)
        return 0;

    std::vector<int_t> x(n);
    std::vector<int_t> y(n);
    std::vector<cost_t> v(n);
    std::vector<int_t> free_rows(n);

    int_t n_actual_free_rows;

    n_actual_free_rows = _ccrrt_dense(n, cost, free_rows, x, y, v);

    int iterations = 0;
    while (n_actual_free_rows > 0 && iterations < 2) {
        if (static_cast<uint_t>(n_actual_free_rows) > n) {
            PRINTF("Error: lapjv_internal - n_actual_free_rows (%d) > n (%u)\n",
                   n_actual_free_rows, n);
            throw std::range_error(
                "lapjv_internal: n_actual_free_rows exceeds n, out of bounds.");
        }
        n_actual_free_rows =
            _carr_dense(n, cost, n_actual_free_rows, free_rows, x, y, v);
        iterations++;
    }

    if (n_actual_free_rows > 0) {
        if (static_cast<uint_t>(n_actual_free_rows) > n) {
            PRINTF("Error: lapjv_internal - n_actual_free_rows (%d) > n (%u) "
                   "before _ca_dense\n",
                   n_actual_free_rows, n);
            throw std::range_error("lapjv_internal: n_actual_free_rows "
                                     "exceeds n before _ca_dense.");
        }
        _ca_dense(n, cost, n_actual_free_rows, free_rows, x, y, v);
        n_actual_free_rows = 0;
    }
    for (uint_t i = 0; i < n; ++i) {
        x_data[i] = x[i];
        y_data[i] = y[i];
    }

    return n_actual_free_rows;
}

float computeMaxCostValue(const std::vector<std::vector<float>> &cost_c) {
    if (cost_c.empty()) {
        return 0.0f;
    }
    
    float cost_max = 0.0f;
    bool first = true;
    
    for (const auto &row : cost_c) {
        for (float val_cost : row) {
            if (val_cost >= std::numeric_limits<float>::max()) {
                continue;
            }
            if (first || val_cost > cost_max) {
                cost_max = val_cost;
                first = false;
            }
        }
    }
    
    return first ? 0.0f : cost_max;
}

float computeDefaultValue(const std::vector<std::vector<float>> &cost_c,
                          float cost_limit) {
    if (cost_limit < std::numeric_limits<float>::max() && cost_limit > 0) {
        return cost_limit / 2.0f;
    }
    
    float cost_max = computeMaxCostValue(cost_c);
    return (cost_max == 0.0f) ? 1.0f : cost_max + 1.0f;
}

void fillMatrixWithDefault(std::vector<std::vector<float>> &cost_c_extended,
                           float default_val) {
    for (auto &row : cost_c_extended) {
        std::fill(row.begin(), row.end(), default_val);
    }
}

void fillExtendedRegionWithZero(std::vector<std::vector<float>> &cost_c_extended,
                                int n_rows, int n_cols, int n) {
    for (int i = n_rows; i < n; i++) {
        for (int j = n_cols; j < n; j++) {
            cost_c_extended.at(i).at(j) = 0;
        }
    }
}

void copyOriginalCost(const std::vector<std::vector<float>> &cost_c,
                      std::vector<std::vector<float>> &cost_c_extended,
                      int n_rows, int n_cols, float default_val) {
    for (int i = 0; i < n_rows; i++) {
        for (int j = 0; j < n_cols; j++) {
            if (static_cast<size_t>(i) < cost_c.size() &&
                static_cast<size_t>(j) < cost_c[i].size()) {
                cost_c_extended.at(i).at(j) = cost_c[i][j];
            } else {
                PRINTF("Warning: copyOriginalCost - cost_c access "
                       "out of bounds.\n");
                cost_c_extended.at(i).at(j) = default_val;
            }
        }
    }
}

void initializeExtendedCostMatrix(
    const std::vector<std::vector<float>> &cost_c,
    std::vector<std::vector<float>> &cost_c_extended, int n_rows, int n_cols,
    int n, float cost_limit) {
    float default_val = computeDefaultValue(cost_c, cost_limit);
    fillMatrixWithDefault(cost_c_extended, default_val);
    fillExtendedRegionWithZero(cost_c_extended, n_rows, n_cols, n);
    copyOriginalCost(cost_c, cost_c_extended, n_rows, n_cols, default_val);
}

float computeOptimalCost(const std::vector<int> &rowsol, float *const *cost_ptr,
                         int n_rows_original) {
    float opt = 0.0f;
    for (int i = 0; i < n_rows_original; i++) {
        if (static_cast<size_t>(i) >= rowsol.size())
            break;

        int c = rowsol[i];
        if (c >= 0) {
            opt += cost_ptr[i][c];
        }
    }
    return opt;
}

void validateInputs(const std::vector<std::vector<float>> &cost, int &n_rows,
                    int &n_cols, std::vector<int> &rowsol,
                    std::vector<int> &colsol) {
    if (cost.empty()) {
        rowsol.clear();
        colsol.clear();
        throw std::invalid_argument("Cost matrix is empty.");
    }

    n_rows = static_cast<int>(cost.size());
    n_cols = static_cast<int>(cost[0].size());

    for (size_t i = 1; i < cost.size(); ++i) {
        if (cost[i].size() != static_cast<size_t>(n_cols)) {
            throw std::invalid_argument(
                "Cost matrix rows have inconsistent number of columns.");
        }
    }

    if (n_rows == 0 || n_cols == 0) {
        rowsol.assign(n_rows, -1);
        colsol.assign(n_cols, -1);
        throw std::invalid_argument("Cost matrix has zero dimension.");
    }
    rowsol.assign(n_rows, -1);
    colsol.assign(n_cols, -1);
}

int prepareCostMatrix(const std::vector<std::vector<float>> &cost,
                      std::vector<std::vector<float>> &result_matrix,
                      int n_rows, int n_cols, bool extend_cost,
                      float cost_limit) {
    if (n_rows == n_cols && !extend_cost &&
        cost_limit >= std::numeric_limits<float>::max()) {
        result_matrix = cost;
        return n_rows;
    }

    if (n_rows != n_cols && !extend_cost) {
        throw std::invalid_argument(
            "execLapjv: For non-square matrices that are not being "
            "automatically extended to square, 'extend_cost' must be true (or "
            "handle squaring manually before call).");
    }
    int n_lapjv;
    if (extend_cost) {
        n_lapjv = std::max(n_rows, n_cols);
    } else {
        n_lapjv = n_rows;
    }

    result_matrix.assign(n_lapjv, std::vector<float>(n_lapjv));
    initializeExtendedCostMatrix(cost, result_matrix, n_rows, n_cols, n_lapjv,
                                 cost_limit);
    return n_lapjv;
}

void flattenCostMatrix(const std::vector<std::vector<float>> &matrix,
                       std::vector<float> &flat_cost_storage,
                       std::vector<float *> &cost_ptr_vec, int n) {
    flat_cost_storage.resize(static_cast<size_t>(n) * n);
    cost_ptr_vec.resize(n);

    for (int i = 0; i < n; ++i) {
        cost_ptr_vec[i] = &flat_cost_storage[static_cast<size_t>(i) * n];
        for (int j = 0; j < n; ++j) {
            if (static_cast<size_t>(i) < matrix.size() &&
                static_cast<size_t>(j) < matrix[i].size()) {
                flat_cost_storage[static_cast<size_t>(i) * n + j] =
                    matrix[i][j];
            } else {
                PRINTF("Warning: flattenCostMatrix - access out of bounds for "
                       "input 'matrix'.\n");
                flat_cost_storage[static_cast<size_t>(i) * n + j] =
                    std::numeric_limits<float>::max();
            }
        }
    }
}

void runLapjvAndPostprocess(int n_lapjv, float *const *cost_matrix_ptr,
                            std::vector<int> &x_solution,
                            std::vector<int> &y_solution, int n_rows_original,
                            int n_cols_original) {

    int ret = lapjv_internal(n_lapjv, cost_matrix_ptr, x_solution.data(),
                             y_solution.data());
    if (ret != 0) {
        PRINTF("Error: execLapjv - lapjv_internal() failed with code %d.\n",
               ret);
        throw std::domain_error("execLapjv: lapjv_internal() failed.");
    }
    if (n_lapjv > n_rows_original || n_lapjv > n_cols_original) {
        for (int i = 0; i < n_lapjv; ++i) {
            if (static_cast<size_t>(i) < x_solution.size() &&
                x_solution[i] >= n_cols_original) {
                x_solution[i] = -1;
            }
            if (static_cast<size_t>(i) < y_solution.size() &&
                y_solution[i] >= n_rows_original) {
                y_solution[i] = -1;
            }
        }
    }
}

void updateSolutions(const std::vector<int> &x_lapjv_sol,
                     const std::vector<int> &y_lapjv_sol,
                     std::vector<int> &final_rowsol,
                     std::vector<int> &final_colsol, int n_rows_original,
                     int n_cols_original) {
    for (int i = 0; i < n_rows_original; ++i) {
        if (static_cast<size_t>(i) < x_lapjv_sol.size()) {
            final_rowsol.at(i) = x_lapjv_sol[i];
        } else {
            final_rowsol.at(i) = -1;
        }
    }
    for (int i = 0; i < n_cols_original; ++i) {
        if (static_cast<size_t>(i) < y_lapjv_sol.size()) {
            final_colsol.at(i) = y_lapjv_sol[i];
        } else {
            final_colsol.at(i) = -1;
        }
    }
}

float execLapjv(const std::vector<std::vector<float>> &cost,
                std::vector<int> &rowsol, std::vector<int> &colsol,
                bool extend_cost /*= true*/, float cost_limit /*= LARGE*/,
                bool return_cost /*= true*/) {
    int n_rows;
    int n_cols;

    try {
        validateInputs(cost, n_rows, n_cols, rowsol, colsol);
    } catch (const std::invalid_argument &e) {
        PRINTF("Validation failed: %s\n", e.what());
        (void)e; // Suppress unused warning in non-debug builds
        return 0.0f;
    }

    std::vector<std::vector<float>> cost_matrix_prepared;
    int n_lapjv = prepareCostMatrix(cost, cost_matrix_prepared, n_rows, n_cols,
                                    extend_cost, cost_limit);

    if (n_lapjv == 0) {
        rowsol.assign(n_rows, -1);
        colsol.assign(n_cols, -1);
        return 0.0f;
    }

    std::vector<float> flat_cost_storage;
    std::vector<float *> cost_ptr_vec;
    flattenCostMatrix(cost_matrix_prepared, flat_cost_storage, cost_ptr_vec,
                      n_lapjv);

    std::vector<int> x_lapjv_sol(n_lapjv);
    std::vector<int> y_lapjv_sol(n_lapjv);

    try {
        runLapjvAndPostprocess(n_lapjv, cost_ptr_vec.data(), x_lapjv_sol,
                               y_lapjv_sol, n_rows, n_cols);
    } catch (const std::invalid_argument &e) {
        PRINTF("LAPJV invalid argument: %s\n", e.what());
        (void)e;
        return 0.0f;
    } catch (const std::domain_error &e) {
        PRINTF("LAPJV domain error: %s\n", e.what());
        (void)e;
        return 0.0f;
    } catch (const std::range_error &e) {
        PRINTF("LAPJV range error: %s\n", e.what());
        (void)e;
        return 0.0f;
    } catch (const std::overflow_error &e) {
        PRINTF("LAPJV overflow: %s\n", e.what());
        (void)e;
        return 0.0f;
    } catch (const std::out_of_range &e) {
        PRINTF("LAPJV out of range: %s\n", e.what());
        (void)e;
        return 0.0f;
    }
    updateSolutions(x_lapjv_sol, y_lapjv_sol, rowsol, colsol, n_rows, n_cols);

    if (!return_cost) {
        return 0.0f;
    }
    return computeOptimalCost(
        rowsol, const_cast<float *const *>(cost_ptr_vec.data()), n_rows);
}