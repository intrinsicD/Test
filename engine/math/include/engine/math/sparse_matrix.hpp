#pragma once

#include "engine/math/matrix.hpp"

#include <vector>
#include <algorithm>
#include <cstddef>
#include <cassert>
#include <optional>
#include <limits>

namespace engine::math
{
    // Sparse, column-major (CSC) matrix with dynamic dimensions.
    // Storage:
    //   col_ptr: size = cols + 1, col_ptr[c]..col_ptr[c+1]-1 are indices of column c
    //   row_ind: size = nnz, row index for each value
    //   values : size = nnz, value for each nonzero
    template <typename T>
    struct SparseMatrix
    {
        using value_type = T;
        using size_type = std::size_t;
        using index_type = std::size_t;

        // --- data ---
        size_type rows_ = 0;
        size_type cols_ = 0;
        std::vector<index_type> col_ptr; // size cols_ + 1
        std::vector<index_type> row_ind; // size nnz
        std::vector<value_type> values; // size nnz

        // --- basic ctors ---
        ENGINE_MATH_INLINE SparseMatrix() noexcept = default;

        ENGINE_MATH_INLINE SparseMatrix(size_type r, size_type c)
            : rows_(r), cols_(c), col_ptr(c + 1, 0)
        {
        }

        // shape
        ENGINE_MATH_INLINE size_type rows() const noexcept { return rows_; }
        ENGINE_MATH_INLINE size_type cols() const noexcept { return cols_; }
        ENGINE_MATH_INLINE size_type nnz() const noexcept { return values.size(); }

        // reset to empty with same shape
        ENGINE_MATH_INLINE void clear()
        {
            std::fill(col_ptr.begin(), col_ptr.end(), 0);
            if (!col_ptr.empty()) col_ptr.back() = 0;
            row_ind.clear();
            values.clear();
        }

        ENGINE_MATH_INLINE void reserve(size_type nonzeros)
        {
            row_ind.reserve(nonzeros);
            values.reserve(nonzeros);
        }

        // --- Triplet builder (COO -> CSC) ---
        struct Triplet
        {
            size_type row, col;
            value_type val;
        };

        // Build from triplets (allows duplicates -> summed).
        // Complexity: O(nnz log nnz) due to sorts.
        static ENGINE_MATH_INLINE SparseMatrix from_triplets(
            size_type rows, size_type cols,
            std::vector<Triplet> trips,
            bool sum_duplicates = true)
        {
            SparseMatrix A(rows, cols);
            if (rows == 0 || cols == 0 || trips.empty())
            {
                A.col_ptr.assign(cols + 1, 0);
                return A;
            }

            // sort by (col, row)
            std::sort(trips.begin(), trips.end(), [](const Triplet& a, const Triplet& b)
            {
                if (a.col != b.col) return a.col < b.col;
                return a.row < b.row;
            });

            A.col_ptr.assign(cols + 1, 0);

            // count per column (after optional merging)
            std::vector<Triplet> merged;
            merged.reserve(trips.size());

            if (sum_duplicates)
            {
                size_type i = 0;
                while (i < trips.size())
                {
                    size_type j = i + 1;
                    Triplet cur = trips[i];
                    while (j < trips.size() && trips[j].col == cur.col && trips[j].row == cur.row)
                    {
                        cur.val += trips[j].val;
                        ++j;
                    }
                    if (cur.val != T{}) merged.push_back(cur); // drop explicit zeros
                    i = j;
                }
            }
            else
            {
                // keep all (and drop zeros)
                for (const auto& t : trips) if (t.val != T{}) merged.push_back(t);
            }

            // fill col_ptr counts
            for (const auto& t : merged)
            {
                assert(t.row < rows && t.col < cols);
                ++A.col_ptr[t.col + 1];
            }
            // prefix sum
            for (size_t c = 0; c < cols; ++c)
            {
                A.col_ptr[c + 1] += A.col_ptr[c];
            }

            A.row_ind.resize(merged.size());
            A.values.resize(merged.size());

            // temp write cursor per column
            std::vector<index_type> next = A.col_ptr;
            for (const auto& t : merged)
            {
                const auto pos = next[t.col]++;
                A.row_ind[pos] = t.row;
                A.values[pos] = t.val;
            }
            return A;
        }

        // Iterate all nonzeros: f(row, col, value)
        template <typename Fn>
        ENGINE_MATH_INLINE void for_each_nz(Fn&& f) const
        {
            for (size_type c = 0; c < cols_; ++c)
            {
                for (index_type k = col_ptr[c]; k < col_ptr[c + 1]; ++k)
                {
                    f(row_ind[k], c, values[k]);
                }
            }
        }

        // Optional getter (O(nnz_col) binary search in the column)
        ENGINE_MATH_INLINE std::optional<value_type> try_get(size_type r, size_type c) const noexcept
        {
            assert(r < rows_ && c < cols_);
            const index_type begin = col_ptr[c], end = col_ptr[c + 1];
            auto it = std::lower_bound(row_ind.begin() + begin, row_ind.begin() + end, r);
            if (it != row_ind.begin() + end && *it == r)
            {
                size_type idx = static_cast<size_type>(it - row_ind.begin());
                return values[idx];
            }
            return std::nullopt;
        }

        // Insert or overwrite (keeps CSC sorted by row within column).
        // Amortized O(nnz) worst-case due to vector insertions; good for sparse edits.
        ENGINE_MATH_INLINE void set(size_type r, size_type c, const value_type& v)
        {
            assert(r < rows_ && c < cols_);
            index_type begin = col_ptr[c], end = col_ptr[c + 1];
            auto it = std::lower_bound(row_ind.begin() + begin, row_ind.begin() + end, r);
            if (it != row_ind.begin() + end && *it == r)
            {
                // overwrite
                values[static_cast<size_type>(it - row_ind.begin())] = v;
                return;
            }
            // insert new
            index_type pos = static_cast<index_type>(it - row_ind.begin());
            row_ind.insert(row_ind.begin() + pos, r);
            values.insert(values.begin() + pos, v);
            // bump subsequent col_ptr
            for (size_type cc = c + 1; cc < col_ptr.size(); ++cc) ++col_ptr[cc];
        }

        // Add delta to entry (creates if missing)
        ENGINE_MATH_INLINE void add_to(size_type r, size_type c, const value_type& delta)
        {
            assert(r < rows_ && c < cols_);
            index_type begin = col_ptr[c], end = col_ptr[c + 1];
            auto it = std::lower_bound(row_ind.begin() + begin, row_ind.begin() + end, r);
            if (it != row_ind.begin() + end && *it == r)
            {
                values[static_cast<size_type>(it - row_ind.begin())] += delta;
                return;
            }
            // insert new
            index_type pos = static_cast<index_type>(it - row_ind.begin());
            row_ind.insert(row_ind.begin() + pos, r);
            values.insert(values.begin() + pos, delta);
            for (size_type cc = c + 1; cc < col_ptr.size(); ++cc) ++col_ptr[cc];
        }

        // y = A * x  (x.size() == cols), returns vector of size rows
        ENGINE_MATH_INLINE std::vector<value_type> operator*(const std::vector<value_type>& x) const
        {
            assert(x.size() == cols_);
            std::vector<value_type> y(rows_, value_type{});
            for (size_type c = 0; c < cols_; ++c)
            {
                const value_type xc = x[c];
                if (xc == value_type{}) continue;
                const index_type b = col_ptr[c], e = col_ptr[c + 1];
                for (index_type k = b; k < e; ++k)
                {
                    y[row_ind[k]] += values[k] * xc;
                }
            }
            return y;
        }

        // y += A * x  (accumulate)
        ENGINE_MATH_INLINE void multiply_accumulate(const std::vector<value_type>& x,
                                                    std::vector<value_type>& y) const
        {
            assert(x.size() == cols_ && y.size() == rows_);
            for (size_type c = 0; c < cols_; ++c)
            {
                const value_type xc = x[c];
                if (xc == value_type{}) continue;
                const index_type b = col_ptr[c], e = col_ptr[c + 1];
                for (index_type k = b; k < e; ++k)
                {
                    y[row_ind[k]] += values[k] * xc;
                }
            }
        }

        // Simple structure checkers
        ENGINE_MATH_INLINE bool is_column_sorted() const noexcept
        {
            for (size_type c = 0; c + 1 < col_ptr.size(); ++c)
            {
                if (col_ptr[c] > col_ptr[c + 1]) return false;
                auto b = row_ind.begin() + col_ptr[c];
                auto e = row_ind.begin() + col_ptr[c + 1];
                if (!std::is_sorted(b, e)) return false;
            }
            return true;
        }

        // A^T
        ENGINE_MATH_INLINE SparseMatrix transpose() const
        {
            SparseMatrix AT(cols_, rows_);
            AT.col_ptr.assign(rows_ + 1, 0);
            // count entries per row -> columns of AT
            for (index_type k = 0; k < static_cast<index_type>(row_ind.size()); ++k)
            {
                ++AT.col_ptr[row_ind[k] + 1];
            }
            for (size_type r = 0; r < rows_; ++r) AT.col_ptr[r + 1] += AT.col_ptr[r];

            AT.row_ind.resize(values.size());
            AT.values.resize(values.size());

            std::vector<index_type> next = AT.col_ptr;
            for (size_type c = 0; c < cols_; ++c)
            {
                for (index_type k = col_ptr[c]; k < col_ptr[c + 1]; ++k)
                {
                    const size_type r = row_ind[k];
                    const index_type pos = next[r]++;
                    AT.row_ind[pos] = c; // column becomes row
                    AT.values[pos] = values[k];
                }
            }
            return AT;
        }

        // A += B (same shape). Implemented via triplet merge (robust & simple).
        ENGINE_MATH_INLINE SparseMatrix& operator+=(const SparseMatrix& B)
        {
            assert(rows_ == B.rows_ && cols_ == B.cols_);
            std::vector<Triplet> trips;
            trips.reserve(nnz() + B.nnz());
            this->for_each_nz([&](size_type r, size_type c, const value_type& v)
            {
                trips.push_back({r, c, v});
            });
            B.for_each_nz([&](size_type r, size_type c, const value_type& v)
            {
                trips.push_back({r, c, v});
            });
            *this = from_triplets(rows_, cols_, std::move(trips), /*sum_duplicates*/true);
            return *this;
        }

        ENGINE_MATH_INLINE SparseMatrix& operator-=(const SparseMatrix& B)
        {
            assert(rows_ == B.rows_ && cols_ == B.cols_);
            std::vector<Triplet> trips;
            trips.reserve(nnz() + B.nnz());
            this->for_each_nz([&](size_type r, size_type c, const value_type& v)
            {
                trips.push_back({r, c, v});
            });
            B.for_each_nz([&](size_type r, size_type c, const value_type& v)
            {
                trips.push_back({r, c, -v});
            });
            *this = from_triplets(rows_, cols_, std::move(trips), /*sum_duplicates*/true);
            return *this;
        }

        ENGINE_MATH_INLINE SparseMatrix& operator*=(value_type s)
        {
            if (s == value_type{})
            {
                values.assign(values.size(), value_type{});
                // optional: drop explicit zeros (we keep them; caller can prune)
            }
            else if (s != value_type{1})
            {
                for (auto& v : values) v *= s;
            }
            return *this;
        }

        // Prunes explicit zeros (exact compare). Useful after *= 0 or cancellations.
        ENGINE_MATH_INLINE void prune_zeros()
        {
            for (size_type c = 0; c < cols_; ++c)
            {
                index_type write = col_ptr[c];
                for (index_type k = col_ptr[c]; k < col_ptr[c + 1]; ++k)
                {
                    if (values[k] != value_type{})
                    {
                        row_ind[write] = row_ind[k];
                        values[write] = values[k];
                        ++write;
                    }
                }
                col_ptr[c + 1] = write;
            }
            row_ind.resize(col_ptr.back());
            values.resize(col_ptr.back());
        }
    };

    // Free operators
    template <typename T>
    ENGINE_MATH_INLINE SparseMatrix<T> operator+(SparseMatrix<T> A, const SparseMatrix<T>& B)
    {
        A += B;
        return A;
    }

    template <typename T>
    ENGINE_MATH_INLINE SparseMatrix<T> operator-(SparseMatrix<T> A, const SparseMatrix<T>& B)
    {
        A -= B;
        return A;
    }

    template <typename T>
    ENGINE_MATH_INLINE SparseMatrix<T> operator*(SparseMatrix<T> A, T s)
    {
        A *= s;
        return A;
    }

    template <typename T>
    ENGINE_MATH_INLINE SparseMatrix<T> operator*(T s, SparseMatrix<T> B)
    {
        B *= s;
        return B;
    }
} // namespace engine::math
