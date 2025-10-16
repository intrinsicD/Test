#pragma once

#include "engine/core/diagnostics/error.hpp"

#include <cassert>
#include <optional>
#include <type_traits>
#include <utility>
#include <variant>

namespace engine
{
    template <typename T, typename E = ErrorCode>
    class [[nodiscard]] Result
    {
    public:
        using value_type = T;
        using error_type = E;

        constexpr Result(const T& value)
            : storage_{value}
        {
        }

        constexpr Result(T&& value) noexcept(std::is_nothrow_move_constructible_v<T>)
            : storage_{std::move(value)}
        {
        }

        constexpr Result(const E& error)
            : storage_{error}
        {
        }

        constexpr Result(E&& error) noexcept(std::is_nothrow_move_constructible_v<E>)
            : storage_{std::move(error)}
        {
        }

        [[nodiscard]] bool has_value() const noexcept
        {
            return std::holds_alternative<T>(storage_);
        }

        [[nodiscard]] explicit operator bool() const noexcept
        {
            return has_value();
        }

        [[nodiscard]] T& value() &
        {
            assert(has_value());
            return std::get<T>(storage_);
        }

        [[nodiscard]] const T& value() const&
        {
            assert(has_value());
            return std::get<T>(storage_);
        }

        [[nodiscard]] T&& value() &&
        {
            assert(has_value());
            return std::get<T>(std::move(storage_));
        }

        template <typename U>
        [[nodiscard]] T value_or(U&& fallback) const&
        {
            if (has_value())
            {
                return std::get<T>(storage_);
            }

            return static_cast<T>(std::forward<U>(fallback));
        }

        [[nodiscard]] E& error() &
        {
            assert(!has_value());
            return std::get<E>(storage_);
        }

        [[nodiscard]] const E& error() const&
        {
            assert(!has_value());
            return std::get<E>(storage_);
        }

        [[nodiscard]] E&& error() &&
        {
            assert(!has_value());
            return std::get<E>(std::move(storage_));
        }

    private:
        std::variant<T, E> storage_;
    };

    template <typename E>
    class [[nodiscard]] Result<void, E>
    {
    public:
        using value_type = void;
        using error_type = E;

        constexpr Result() noexcept = default;

        constexpr Result(const E& error)
            : error_{error}
        {
        }

        constexpr Result(E&& error) noexcept(std::is_nothrow_move_constructible_v<E>)
            : error_{std::move(error)}
        {
        }

        [[nodiscard]] bool has_value() const noexcept
        {
            return !error_.has_value();
        }

        [[nodiscard]] explicit operator bool() const noexcept
        {
            return has_value();
        }

        void value() const
        {
            assert(has_value());
        }

        [[nodiscard]] E& error() &
        {
            assert(!has_value());
            return error_.value();
        }

        [[nodiscard]] const E& error() const&
        {
            assert(!has_value());
            return error_.value();
        }

        [[nodiscard]] E&& error() &&
        {
            assert(!has_value());
            return std::move(error_.value());
        }

    private:
        std::optional<E> error_{};
    };
} // namespace engine

