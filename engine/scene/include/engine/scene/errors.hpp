#pragma once

#include "engine/core/diagnostics/error.hpp"
#include "engine/core/diagnostics/result.hpp"

#include <string>
#include <string_view>

namespace engine::scene
{
    enum class SceneGraphError : int
    {
        cycle_detected = 1
    };

    [[nodiscard]] constexpr std::string_view to_string(SceneGraphError error) noexcept
    {
        switch (error)
        {
        case SceneGraphError::cycle_detected:
            return "cycle_detected";
        }

        return "unknown";
    }

    class SceneGraphErrorCode final : public engine::EnumeratedErrorCode<SceneGraphError>
    {
    public:
        using EnumeratedErrorCode::EnumeratedErrorCode;

        [[nodiscard]] SceneGraphErrorCode with_message(std::string message) const
        {
            SceneGraphErrorCode copy{*this};
            copy.assign_message(std::move(message));
            return copy;
        }
    };

    [[nodiscard]] inline SceneGraphErrorCode make_scene_graph_error(SceneGraphError error,
                                                                    std::string message = {})
    {
        SceneGraphErrorCode code{"engine.scene", error, to_string(error)};
        if (!message.empty())
        {
            code = code.with_message(std::move(message));
        }
        return code;
    }

    template <typename T>
    using SceneGraphResult = engine::Result<T, SceneGraphErrorCode>;
} // namespace engine::scene