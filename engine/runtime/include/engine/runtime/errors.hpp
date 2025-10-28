#pragma once

#include "engine/core/diagnostics/error.hpp"
#include "engine/core/diagnostics/result.hpp"

#include <string>
#include <string_view>
#include <utility>

namespace engine::runtime
{
    enum class RuntimeError : int
    {
        dependency_invalid_mesh = 1,
        dependency_invalid_binding,
        dependency_invalid_clip,
        dependency_cycle,
        configuration_io_error,
        configuration_parse_error,
        configuration_validation_error
    };

    [[nodiscard]] constexpr std::string_view to_string(RuntimeError error) noexcept
    {
        switch (error)
        {
        case RuntimeError::dependency_invalid_mesh:
            return "dependency_invalid_mesh";
        case RuntimeError::dependency_invalid_binding:
            return "dependency_invalid_binding";
        case RuntimeError::dependency_invalid_clip:
            return "dependency_invalid_clip";
        case RuntimeError::dependency_cycle:
            return "dependency_cycle";
        case RuntimeError::configuration_io_error:
            return "configuration_io_error";
        case RuntimeError::configuration_parse_error:
            return "configuration_parse_error";
        case RuntimeError::configuration_validation_error:
            return "configuration_validation_error";
        }
        return "unknown";
    }

    class RuntimeErrorCode final : public engine::EnumeratedErrorCode<RuntimeError>
    {
    public:
        using EnumeratedErrorCode::EnumeratedErrorCode;

        [[nodiscard]] RuntimeErrorCode with_message(std::string message) const
        {
            RuntimeErrorCode copy{*this};
            copy.assign_message(std::move(message));
            return copy;
        }
    };

    [[nodiscard]] inline RuntimeErrorCode make_runtime_error(RuntimeError error,
                                                             std::string message = {})
    {
        RuntimeErrorCode code{"engine.runtime", error, to_string(error)};
        if (!message.empty())
        {
            code = code.with_message(std::move(message));
        }
        return code;
    }

    template <typename T>
    using RuntimeResult = engine::Result<T, RuntimeErrorCode>;

    using RuntimeValidationResult = RuntimeResult<void>;
} // namespace engine::runtime
