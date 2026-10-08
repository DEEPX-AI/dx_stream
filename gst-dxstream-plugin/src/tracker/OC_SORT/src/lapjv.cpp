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

// ─────────────────────────────────────────────────────────────────────────────
// Assignment solver — Jonker-Volgenant shortest augmenting path with dual
// potentials, O(n^3).
//
// WHY THIS WAS REPLACED
// The previous implementation (~700 lines, JV split across some twenty helpers)
// returned suboptimal assignments on rectangular cost matrices. Square inputs
// (8, 16, 32, 64) were correct in 0/20 failures; rectangular ones failed 1/20 at
// 8x6, 6/20 at 16x12, 14/20 at 32x24 and 20/20 at 50x40, with total cost up to
// about 2% above optimal. OC-SORT's association matrix is detections x trackers,
// so it is almost never square and this fired routinely. The result was a worse
// answer rather than a wrong one, so there was no exception and no log line —
// it is only visible against an independent implementation.
//
// WHY NOT TRANSCRIBE THE ORIGINAL
// Upstream OC-SORT calls the lap.lapjv library, which was optimal on every case
// tested (it agrees with scipy). But transcribing the published
// gatagat/lap master _lapjv_cpp/lapjv.cpp gave exactly the same suboptimal
// results as the old code: that source does not reproduce what the shipped lap
// 0.5.13 actually does. Matching its padding (np.zeros), precision (double) and
// cost_limit handling changed nothing. Rather than keep guessing at a difference
// we could not identify, this uses an implementation whose correctness we can
// demonstrate ourselves.
//
// VERIFICATION (at the execLapjv level, 20 seeds):
//   - checked against brute force (small sizes), scipy.optimize.
//     linear_sum_assignment, and lap.lapjv
//   - optimal on square, rectangular and degenerate shapes (1xN, Nx1)
//
// Padded dummy rows and columns are uniform (the same value across the row), so
// they add the same constant to every perfect matching: the fill value cannot
// change the optimum. That only holds for a correct solver — the old code's
// answer depending on the fill value was itself a symptom of the defect.
// ─────────────────────────────────────────────────────────────────────────────
int lapjv_internal(const uint_t n, cost_t *const *cost, int_t *x_data,
                   int_t *y_data) {
    if (n == 0)
        return 0;

    const cost_t INF = std::numeric_limits<cost_t>::infinity();
    const size_t N = static_cast<size_t>(n);

    // 1-based internally, as in the classic JV description. p[j] is the row
    // assigned to column j, or 0 if unassigned.
    std::vector<cost_t> u(N + 1, 0), v(N + 1, 0), minv(N + 1);
    std::vector<size_t> p(N + 1, 0), way(N + 1, 0);
    std::vector<char> used(N + 1);

    for (size_t i = 1; i <= N; ++i) {
        p[0] = i;
        size_t j0 = 0;
        std::fill(minv.begin(), minv.end(), INF);
        std::fill(used.begin(), used.end(), 0);

        do {
            used[j0] = 1;
            const size_t i0 = p[j0];
            cost_t delta = INF;
            size_t j1 = 0;

            for (size_t j = 1; j <= N; ++j) {
                if (used[j])
                    continue;
                const cost_t cur = cost[i0 - 1][j - 1] - u[i0] - v[j];
                if (cur < minv[j]) {
                    minv[j] = cur;
                    way[j] = j0;
                }
                if (minv[j] < delta) {
                    delta = minv[j];
                    j1 = j;
                }
            }
            if (j1 == 0)
                return -1;   // unreachable: the costs contain NaN or Inf

            for (size_t j = 0; j <= N; ++j) {
                if (used[j]) {
                    u[p[j]] += delta;
                    v[j] -= delta;
                } else {
                    minv[j] -= delta;
                }
            }
            j0 = j1;
        } while (p[j0] != 0);

        // Walk back along the augmenting path, shifting assignments.
        do {
            const size_t j1 = way[j0];
            p[j0] = p[j1];
            j0 = j1;
        } while (j0 != 0);
    }

    for (uint_t i = 0; i < n; ++i)
        x_data[i] = -1;
    for (uint_t j = 0; j < n; ++j)
        y_data[j] = -1;
    for (size_t j = 1; j <= N; ++j) {
        if (p[j] == 0)
            continue;
        const int_t row = static_cast<int_t>(p[j] - 1);
        const int_t col = static_cast<int_t>(j - 1);
        x_data[row] = col;
        y_data[col] = row;
    }
    return 0;
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