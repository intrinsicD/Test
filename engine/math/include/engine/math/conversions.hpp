#pragma once

#include "engine/math/matrix.hpp"
#include "engine/math/vector.hpp"

#include <array>
#include <cassert>
#include <span>

namespace engine::math::conversions
{
    template <typename T, std::size_t N>
    [[nodiscard]] constexpr std::array<T, N> to_array(const Vector<T, N>& value) noexcept
    {
        std::array<T, N> result{};
        for (std::size_t i = 0; i < N; ++i)
        {
            result[i] = value[i];
        }
        return result;
    }

    template <typename T, std::size_t N>
    [[nodiscard]] constexpr Vector<T, N> vector_from_array(const std::array<T, N>& data) noexcept
    {
        Vector<T, N> result{};
        for (std::size_t i = 0; i < N; ++i)
        {
            result[i] = data[i];
        }
        return result;
    }

    template <typename T, std::size_t N>
    [[nodiscard]] constexpr Vector<T, N> vector_from_span(std::span<const T> data) noexcept
    {
        assert(data.size() == N && "vector_from_span expects exactly N elements");
        Vector<T, N> result{};
        for (std::size_t i = 0; i < N; ++i)
        {
            result[i] = static_cast<T>(data[i]);
        }
        return result;
    }

    template <typename T, std::size_t Rows, std::size_t Cols>
    [[nodiscard]] constexpr std::array<T, Rows * Cols> to_column_major_array(
        const Matrix<T, Rows, Cols>& value) noexcept
    {
        std::array<T, Rows * Cols> result{};
        std::size_t index = 0;
        for (std::size_t column = 0; column < Cols; ++column)
        {
            for (std::size_t row = 0; row < Rows; ++row)
            {
                result[index++] = value[row][column];
            }
        }
        return result;
    }

    template <typename T, std::size_t Rows, std::size_t Cols>
    [[nodiscard]] constexpr std::array<T, Rows * Cols> to_row_major_array(const Matrix<T, Rows, Cols>& value) noexcept
    {
        std::array<T, Rows * Cols> result{};
        std::size_t index = 0;
        for (std::size_t row = 0; row < Rows; ++row)
        {
            for (std::size_t column = 0; column < Cols; ++column)
            {
                result[index++] = value[row][column];
            }
        }
        return result;
    }

    template <typename T, std::size_t Rows, std::size_t Cols>
    [[nodiscard]] constexpr Matrix<T, Rows, Cols> matrix_from_column_major_span(std::span<const T> data) noexcept
    {
        assert(data.size() == Rows * Cols && "matrix_from_column_major_span expects Rows*Cols elements");
        Matrix<T, Rows, Cols> result{};
        std::size_t index = 0;
        for (std::size_t column = 0; column < Cols; ++column)
        {
            for (std::size_t row = 0; row < Rows; ++row)
            {
                result[row][column] = static_cast<T>(data[index++]);
            }
        }
        return result;
    }

    template <typename T, std::size_t Rows, std::size_t Cols>
    [[nodiscard]] constexpr Matrix<T, Rows, Cols> matrix_from_row_major_span(std::span<const T> data) noexcept
    {
        assert(data.size() == Rows * Cols && "matrix_from_row_major_span expects Rows*Cols elements");
        Matrix<T, Rows, Cols> result{};
        std::size_t index = 0;
        for (std::size_t row = 0; row < Rows; ++row)
        {
            for (std::size_t column = 0; column < Cols; ++column)
            {
                result[row][column] = static_cast<T>(data[index++]);
            }
        }
        return result;
    }

    template <typename T, std::size_t Rows, std::size_t Cols>
    [[nodiscard]] constexpr Matrix<T, Rows, Cols> matrix_from_column_major_array(
        const std::array<T, Rows * Cols>& data) noexcept
    {
        return matrix_from_column_major_span<T, Rows, Cols>(std::span<const T, Rows * Cols>{data});
    }

    template <typename T, std::size_t Rows, std::size_t Cols>
    [[nodiscard]] constexpr Matrix<T, Rows, Cols> matrix_from_row_major_array(
        const std::array<T, Rows * Cols>& data) noexcept
    {
        return matrix_from_row_major_span<T, Rows, Cols>(std::span<const T, Rows * Cols>{data});
    }
} // namespace engine::math::conversions