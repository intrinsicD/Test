#pragma once

#include "engine/core/diagnostics/error.hpp"
#include "engine/core/diagnostics/result.hpp"
#include "engine/geometry/export.hpp"

#include <string>
#include <string_view>

namespace engine::geometry
{
    enum class RemeshError : int
    {
        invalid_input_mesh = 1,
        invalid_target_configuration,
        invalid_attribute_policy,
        invalid_parameterization,
        telemetry_unavailable,
        unsupported_mode,
        not_implemented
    };

    [[nodiscard]] constexpr std::string_view to_string(RemeshError error) noexcept
    {
        switch (error)
        {
        case RemeshError::invalid_input_mesh:
            return "invalid_input_mesh";
        case RemeshError::invalid_target_configuration:
            return "invalid_target_configuration";
        case RemeshError::invalid_attribute_policy:
            return "invalid_attribute_policy";
        case RemeshError::invalid_parameterization:
            return "invalid_parameterization";
        case RemeshError::telemetry_unavailable:
            return "telemetry_unavailable";
        case RemeshError::unsupported_mode:
            return "unsupported_mode";
        case RemeshError::not_implemented:
            return "not_implemented";
        }

        return "unknown";
    }

    class ENGINE_GEOMETRY_API RemeshErrorCode final : public engine::EnumeratedErrorCode<RemeshError>
    {
    public:
        using EnumeratedErrorCode::EnumeratedErrorCode;

        [[nodiscard]] RemeshErrorCode with_message(std::string message) const
        {
            RemeshErrorCode copy{*this};
            copy.assign_message(std::move(message));
            return copy;
        }
    };

    [[nodiscard]] inline RemeshErrorCode make_remesh_error(RemeshError error, std::string message = {})
    {
        RemeshErrorCode code{"engine.geometry.remesh", error, to_string(error)};
        if (!message.empty())
        {
            code = code.with_message(std::move(message));
        }
        return code;
    }

    template <typename T>
    using RemeshResult = engine::Result<T, RemeshErrorCode>;

    using RemeshValidationResult = engine::Result<void, RemeshErrorCode>;
} // namespace engine::geometry