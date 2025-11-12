#include "engine/rendering/backend/opengl/immediate_command_stream.hpp"

#include <type_traits>
#include <utility>
#include <variant>

#ifndef ENGINE_RENDERING_HAS_GLAD
#    define ENGINE_RENDERING_HAS_GLAD 0
#endif

#if ENGINE_RENDERING_HAS_GLAD
#    include <glad/gl.h>
#endif

#include "engine/rendering/backend/opengl/render_resource_provider.hpp"

namespace engine::rendering::backend::opengl
{
    using engine::rendering::EncodedCommand;

    OpenGLImmediateCommandStream::OpenGLImmediateCommandStream(OpenGLRenderResourceProvider& render_resources) noexcept
        : render_resources_(&render_resources)
    {
    }

    void OpenGLImmediateCommandStream::begin_submission(const OpenGLSubmission& submission)
    {
        current_commands_ = &submission.commands;
        draw_calls_ = 0;
        compute_dispatches_ = 0;
        waited_timelines_.clear();
        signalled_timelines_.clear();
        signalled_fences_.clear();
    }

    void OpenGLImmediateCommandStream::wait_timeline(const OpenGLTimelineSubmit& submit)
    {
        waited_timelines_.push_back(submit);
    }

    void OpenGLImmediateCommandStream::issue_memory_barrier(std::uint32_t mask)
    {
#if ENGINE_RENDERING_HAS_GLAD
        if (mask != 0U && glad_glMemoryBarrier != nullptr)
        {
            glad_glMemoryBarrier(static_cast<GLbitfield>(mask));
        }
#else
        static_cast<void>(mask);
#endif
    }

    void OpenGLImmediateCommandStream::execute_command_buffer(const OpenGLCommandEncoderSubmit& submit)
    {
        static_cast<void>(submit);
        if (render_resources_ == nullptr || current_commands_ == nullptr)
        {
            return;
        }

        for (const auto& command : *current_commands_)
        {
            if (command.is_draw())
            {
                execute_draw_command(command.geometry_draw());
            }
            else if (command.is_dispatch())
            {
                execute_compute_dispatch(command.compute_dispatch());
            }
        }
    }

    void OpenGLImmediateCommandStream::signal_timeline(const OpenGLTimelineSubmit& submit)
    {
        signalled_timelines_.push_back(submit);
    }

    void OpenGLImmediateCommandStream::signal_fence(resources::FenceNativeHandle fence, std::uint64_t value)
    {
        signalled_fences_.emplace_back(fence, value);
    }

    void OpenGLImmediateCommandStream::end_submission(const OpenGLSubmission& submission)
    {
        static_cast<void>(submission);
        current_commands_ = nullptr;
    }

    void OpenGLImmediateCommandStream::execute_draw_command(const GeometryDrawCommand& command)
    {
        const auto visitor = [this, &command](auto&& geometry_handle)
        {
            using Handle = std::decay_t<decltype(geometry_handle)>;
            if constexpr (std::is_same_v<Handle, std::monostate>)
            {
                return;
            }
            else if constexpr (std::is_same_v<Handle, assets::MeshHandle>)
            {
                if (!geometry_handle.empty())
                {
                    execute_mesh_draw(command);
                }
            }
            else if constexpr (std::is_same_v<Handle, assets::PointCloudHandle>)
            {
                if (!geometry_handle.empty())
                {
                    execute_point_cloud_draw(command);
                }
            }
            else
            {
                // Graphs are currently not backed by OpenGL draw paths.
                static_cast<void>(geometry_handle);
            }
        };

        std::visit(visitor, command.geometry);
    }

    void OpenGLImmediateCommandStream::execute_mesh_draw(const GeometryDrawCommand& command)
    {
        if (render_resources_ == nullptr)
        {
            return;
        }

        const auto mesh_handle = std::get<assets::MeshHandle>(command.geometry);
        render_resources_->require_mesh(mesh_handle);

        const auto* record = render_resources_->mesh(mesh_handle);
        if (record == nullptr)
        {
            return;
        }

        ++draw_calls_;

#if ENGINE_RENDERING_HAS_GLAD
        if (record->vertex_array != 0U && glad_glBindVertexArray != nullptr)
        {
            glad_glBindVertexArray(record->vertex_array);
        }

        const auto index_count = static_cast<GLsizei>(record->indices.size());
        if (index_count > 0 && record->index_buffer != 0U && glad_glDrawElements != nullptr)
        {
            glad_glDrawElements(GL_TRIANGLES, index_count, GL_UNSIGNED_INT, nullptr);
        }
        else if (!record->positions.empty() && glad_glDrawArrays != nullptr)
        {
            const auto vertex_count = static_cast<GLsizei>(record->positions.size());
            glad_glDrawArrays(GL_TRIANGLES, 0, vertex_count);
        }

        if (glad_glBindVertexArray != nullptr)
        {
            glad_glBindVertexArray(0);
        }
#else
        static_cast<void>(command);
#endif
    }

    void OpenGLImmediateCommandStream::execute_point_cloud_draw(const GeometryDrawCommand& command)
    {
        if (render_resources_ == nullptr)
        {
            return;
        }

        const auto handle = std::get<assets::PointCloudHandle>(command.geometry);
        render_resources_->require_point_cloud(handle);

        const auto* record = render_resources_->point_cloud(handle);
        if (record == nullptr)
        {
            return;
        }

        ++draw_calls_;

#if ENGINE_RENDERING_HAS_GLAD
        if (record->vertex_array != 0U && glad_glBindVertexArray != nullptr)
        {
            glad_glBindVertexArray(record->vertex_array);
        }

        const auto vertex_count = static_cast<GLsizei>(record->positions.size());
        if (vertex_count > 0 && glad_glDrawArrays != nullptr)
        {
            glad_glDrawArrays(GL_POINTS, 0, vertex_count);
        }

        if (glad_glBindVertexArray != nullptr)
        {
            glad_glBindVertexArray(0);
        }
#else
        static_cast<void>(command);
#endif
    }

    void OpenGLImmediateCommandStream::execute_compute_dispatch(const ComputeDispatchCommand& command)
    {
        ++compute_dispatches_;

#if ENGINE_RENDERING_HAS_GLAD
        if (glad_glDispatchCompute != nullptr)
        {
            glad_glDispatchCompute(static_cast<GLsizei>(command.group_count_x),
                                   static_cast<GLsizei>(command.group_count_y),
                                   static_cast<GLsizei>(command.group_count_z));
        }
#else
        static_cast<void>(command);
#endif
    }
}
