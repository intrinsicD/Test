#pragma once

#include <string>
#include <string_view>
#include <utility>

namespace engine
{
    class ErrorCode
    {
    public:
        constexpr ErrorCode() noexcept = default;

        ErrorCode(std::string_view domain, int value, std::string_view identifier,
                  std::string_view default_message = {})
            : domain_{domain},
              value_{value},
              identifier_{identifier},
              message_{default_message}
        {
        }

        [[nodiscard]] std::string_view domain() const noexcept
        {
            return domain_;
        }

        [[nodiscard]] int value() const noexcept
        {
            return value_;
        }

        [[nodiscard]] std::string_view identifier() const noexcept
        {
            return identifier_;
        }

        [[nodiscard]] std::string_view message() const noexcept
        {
            if (!message_.empty())
            {
                return message_;
            }

            return identifier_;
        }

        [[nodiscard]] bool has_message() const noexcept
        {
            return !message_.empty();
        }

        [[nodiscard]] ErrorCode with_message(std::string message) const
        {
            ErrorCode copy{*this};
            copy.assign_message(std::move(message));
            return copy;
        }

        friend bool operator==(const ErrorCode& lhs, const ErrorCode& rhs) noexcept
        {
            return lhs.domain_ == rhs.domain_ && lhs.value_ == rhs.value_ &&
                   lhs.identifier_ == rhs.identifier_ && lhs.message_ == rhs.message_;
        }

        friend bool operator!=(const ErrorCode& lhs, const ErrorCode& rhs) noexcept
        {
            return !(lhs == rhs);
        }

    protected:
        void assign_message(std::string message)
        {
            message_ = std::move(message);
        }

        void set_identifier(std::string_view identifier) noexcept
        {
            identifier_ = identifier;
        }

        void set_domain(std::string_view domain) noexcept
        {
            domain_ = domain;
        }

        void set_value(int value) noexcept
        {
            value_ = value;
        }

    private:
        std::string_view domain_{};
        int value_{0};
        std::string_view identifier_{};
        std::string message_{};
    };

    template <typename Enum>
    class EnumeratedErrorCode : public ErrorCode
    {
    public:
        using enum_type = Enum;

        constexpr EnumeratedErrorCode() noexcept = default;

        EnumeratedErrorCode(std::string_view domain, Enum code, std::string_view identifier,
                            std::string_view default_message = {})
            : ErrorCode{domain, static_cast<int>(code), identifier, default_message},
              code_{code}
        {
        }

        [[nodiscard]] Enum code() const noexcept
        {
            return code_;
        }

        [[nodiscard]] EnumeratedErrorCode with_message(std::string message) const
        {
            EnumeratedErrorCode copy{*this};
            copy.assign_message(std::move(message));
            return copy;
        }

    protected:
        using ErrorCode::assign_message;

    private:
        Enum code_{};
    };
} // namespace engine

