#include "engine/rendering/backend/opengl/immediate_command_stream.hpp"

#include <array>
#include <string_view>
#include <type_traits>
#include <utility>
#include <variant>

#ifndef ENGINE_RENDERING_HAS_GLAD
#    define ENGINE_RENDERING_HAS_GLAD 0
#endif

#if ENGINE_RENDERING_HAS_GLAD
#    include <glad/gl.h>
#endif

#include "engine/core/log.hpp"
#include "engine/math/matrix.hpp"
#include "engine/math/transform.hpp"
#include "engine/rendering/backend/opengl/render_resource_provider.hpp"

namespace engine::rendering::backend::opengl
{
    using engine::rendering::EncodedCommand;

    namespace
    {
#if ENGINE_RENDERING_HAS_GLAD
        constexpr std::string_view kForwardVertexShader = R"(
            #version 460 core

            layout(location = 0) in vec3 aPosition;
            layout(location = 1) in vec3 aNormal;

            uniform mat4 uModel;
            uniform mat4 uView;
            uniform mat4 uProjection;

            out vec3 vNormal;
            out vec3 vFragPos;

            void main()
            {
                vec4 world = uModel * vec4(aPosition, 1.0);
                vFragPos = world.xyz;
                mat3 normalMatrix = mat3(transpose(inverse(uModel)));
                vNormal = normalMatrix * aNormal;
                gl_Position = uProjection * uView * world;
            }
        )";

        constexpr std::string_view kForwardFragmentShader = R"(
            #version 460 core

            in vec3 vNormal;
            in vec3 vFragPos;

            out vec4 FragColor;

            uniform vec3 uLightPos;
            uniform vec3 uViewPos;
            uniform vec3 uObjectColor;

            void main()
            {
                vec3 normal = normalize(vNormal);
                vec3 lightDir = normalize(uLightPos - vFragPos);
                float diff = max(dot(normal, lightDir), 0.0);
                vec3 diffuse = diff * uObjectColor;

                vec3 viewDir = normalize(uViewPos - vFragPos);
                vec3 reflectDir = reflect(-lightDir, normal);
                float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32.0);
                vec3 specular = vec3(0.25) * spec;

                vec3 ambient = 0.2 * uObjectColor;
                FragColor = vec4(ambient + diffuse + specular, 1.0);
            }
        )";
#endif
    } // namespace

    OpenGLImmediateCommandStream::OpenGLImmediateCommandStream(OpenGLRenderResourceProvider& render_resources) noexcept
        : render_resources_(&render_resources)
    {
    }

    OpenGLImmediateCommandStream::~OpenGLImmediateCommandStream()
    {
#if ENGINE_RENDERING_HAS_GLAD
        if (shader_program_ != 0U && glad_glDeleteProgram != nullptr)
        {
            glad_glDeleteProgram(shader_program_);
            shader_program_ = 0U;
        }
#endif
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
        if (!ensure_shader_program())
        {
            return;
        }

        upload_draw_uniforms(command);

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
        if (!ensure_shader_program())
        {
            return;
        }

        upload_draw_uniforms(command);

        if (record->vertex_array != 0U && glad_glBindVertexArray != nullptr)
        {
            glad_glBindVertexArray(record->vertex_array);
        }

        const auto vertex_count = static_cast<GLsizei>(record->positions.size());
        if (vertex_count > 0 && glad_glDrawArrays != nullptr)
        {
            if (glad_glPointSize != nullptr)
            {
                glad_glPointSize(4.0F);
            }
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

    bool OpenGLImmediateCommandStream::ensure_shader_program()
    {
#if ENGINE_RENDERING_HAS_GLAD
        if (shader_program_ != 0U)
        {
            return true;
        }

        const auto vertex = compile_shader(GL_VERTEX_SHADER, kForwardVertexShader.data());
        const auto fragment = compile_shader(GL_FRAGMENT_SHADER, kForwardFragmentShader.data());
        if (vertex == 0U || fragment == 0U)
        {
            if (vertex != 0U && glad_glDeleteShader != nullptr)
            {
                glad_glDeleteShader(vertex);
            }
            if (fragment != 0U && glad_glDeleteShader != nullptr)
            {
                glad_glDeleteShader(fragment);
            }
            return false;
        }

        if (glad_glCreateProgram == nullptr)
        {
            return false;
        }

        shader_program_ = glad_glCreateProgram();
        if (shader_program_ == 0U)
        {
            return false;
        }

        glad_glAttachShader(shader_program_, vertex);
        glad_glAttachShader(shader_program_, fragment);
        glad_glLinkProgram(shader_program_);

        if (glad_glGetProgramiv != nullptr)
        {
            GLint linked = 0;
            glad_glGetProgramiv(shader_program_, GL_LINK_STATUS, &linked);
            if (linked == GL_FALSE)
            {
                if (glad_glGetProgramInfoLog != nullptr)
                {
                    char info_log[512];
                    glad_glGetProgramInfoLog(shader_program_, 512, nullptr, info_log);
                    ENGINE_ERROR("OpenGL shader link failed: {}", info_log);
                }
                glad_glDeleteProgram(shader_program_);
                shader_program_ = 0U;
            }
        }

        if (glad_glDeleteShader != nullptr)
        {
            glad_glDeleteShader(vertex);
            glad_glDeleteShader(fragment);
        }

        if (shader_program_ == 0U)
        {
            return false;
        }

        if (glad_glGetUniformLocation != nullptr)
        {
            model_uniform_location_ = glad_glGetUniformLocation(shader_program_, "uModel");
            view_uniform_location_ = glad_glGetUniformLocation(shader_program_, "uView");
            projection_uniform_location_ = glad_glGetUniformLocation(shader_program_, "uProjection");
            light_pos_uniform_location_ = glad_glGetUniformLocation(shader_program_, "uLightPos");
            view_pos_uniform_location_ = glad_glGetUniformLocation(shader_program_, "uViewPos");
            object_color_uniform_location_ = glad_glGetUniformLocation(shader_program_, "uObjectColor");
        }

        return true;
#else
        return false;
#endif
    }

    unsigned int OpenGLImmediateCommandStream::compile_shader(unsigned int type, const char* source)
    {
#if ENGINE_RENDERING_HAS_GLAD
        if (glad_glCreateShader == nullptr)
        {
            return 0U;
        }

        const unsigned int shader = glad_glCreateShader(type);
        if (shader == 0U)
        {
            return 0U;
        }

        glad_glShaderSource(shader, 1, &source, nullptr);
        glad_glCompileShader(shader);

        if (glad_glGetShaderiv != nullptr)
        {
            GLint compiled = 0;
            glad_glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
            if (compiled == GL_FALSE)
            {
                if (glad_glGetShaderInfoLog != nullptr)
                {
                    char info_log[512];
                    glad_glGetShaderInfoLog(shader, 512, nullptr, info_log);
                    ENGINE_ERROR("OpenGL shader compilation failed: {}", info_log);
                }
                if (glad_glDeleteShader != nullptr)
                {
                    glad_glDeleteShader(shader);
                }
                return 0U;
            }
        }

        return shader;
#else
        static_cast<void>(type);
        static_cast<void>(source);
        return 0U;
#endif
    }

    void OpenGLImmediateCommandStream::upload_matrix(int location, const engine::math::mat4& matrix)
    {
#if ENGINE_RENDERING_HAS_GLAD
        if (location < 0 || glad_glUniformMatrix4fv == nullptr)
        {
            return;
        }

        std::array<float, 16> values{};
        std::size_t index = 0;
        for (std::size_t column = 0; column < 4; ++column)
        {
            for (std::size_t row = 0; row < 4; ++row)
            {
                values[index++] = matrix.columns[column][row];
            }
        }

        glad_glUniformMatrix4fv(location, 1, GL_FALSE, values.data());
#else
        static_cast<void>(location);
        static_cast<void>(matrix);
#endif
    }

    void OpenGLImmediateCommandStream::upload_draw_uniforms(const GeometryDrawCommand& command)
    {
#if ENGINE_RENDERING_HAS_GLAD
        if (!ensure_shader_program() || glad_glUseProgram == nullptr)
        {
            return;
        }

        glad_glUseProgram(shader_program_);

        const auto model_matrix = engine::math::to_matrix(command.transform);
        upload_matrix(model_uniform_location_, model_matrix);
        upload_matrix(view_uniform_location_, command.view_matrix);
        upload_matrix(projection_uniform_location_, command.projection_matrix);

        if (light_pos_uniform_location_ >= 0 && glad_glUniform3f != nullptr)
        {
            glad_glUniform3f(light_pos_uniform_location_, 4.0F, 6.0F, 4.0F);
        }

        if (view_pos_uniform_location_ >= 0 && glad_glUniform3f != nullptr)
        {
            glad_glUniform3f(view_pos_uniform_location_,
                             command.camera_position[0],
                             command.camera_position[1],
                             command.camera_position[2]);
        }

        if (object_color_uniform_location_ >= 0 && glad_glUniform3f != nullptr)
        {
            glad_glUniform3f(object_color_uniform_location_, 0.85F, 0.72F, 0.60F);
        }
#else
        static_cast<void>(command);
#endif
    }
}
