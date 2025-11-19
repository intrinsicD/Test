#pragma once

#include <cstddef>
#include <utility>
#include <vector>

#include "engine/rendering/backend/opengl/gpu_scheduler.hpp"
#include "engine/math/matrix.hpp"

namespace engine::rendering::backend::opengl
{
    class OpenGLRenderResourceProvider;

    using engine::rendering::EncodedCommand;

    /**
     * \brief Command stream that immediately executes recorded draw commands against OpenGL.
     *
     * The stream consumes \ref OpenGLSubmission objects emitted by the GPU scheduler and resolves
     * mesh handles through \ref OpenGLRenderResourceProvider before issuing draw calls.  When GLAD
     * is unavailable the stream still records attempted draw counts so higher layers can validate
     * behaviour in headless environments.
     */
    class OpenGLImmediateCommandStream final : public CommandStream
    {
    public:
        explicit OpenGLImmediateCommandStream(OpenGLRenderResourceProvider& render_resources) noexcept;
        ~OpenGLImmediateCommandStream() override;

        void begin_submission(const OpenGLSubmission& submission) override;
        void wait_timeline(const OpenGLTimelineSubmit& submit) override;
        void issue_memory_barrier(std::uint32_t mask) override;
        void execute_command_buffer(const OpenGLCommandEncoderSubmit& submit) override;
        void signal_timeline(const OpenGLTimelineSubmit& submit) override;
        void signal_fence(resources::FenceNativeHandle fence, std::uint64_t value) override;
        void end_submission(const OpenGLSubmission& submission) override;

        [[nodiscard]] std::size_t draw_call_count() const noexcept { return draw_calls_; }
        [[nodiscard]] std::size_t compute_dispatch_count() const noexcept { return compute_dispatches_; }

        [[nodiscard]] const std::vector<OpenGLTimelineSubmit>& waited_timelines() const noexcept
        {
            return waited_timelines_;
        }

        [[nodiscard]] const std::vector<OpenGLTimelineSubmit>& signalled_timelines() const noexcept
        {
            return signalled_timelines_;
        }

        [[nodiscard]] const std::vector<std::pair<resources::FenceNativeHandle, std::uint64_t>>&
        signalled_fences() const noexcept
        {
            return signalled_fences_;
        }

    private:
        OpenGLRenderResourceProvider* render_resources_;
        const std::vector<EncodedCommand>* current_commands_{nullptr};
        std::size_t draw_calls_{0};
        std::size_t compute_dispatches_{0};
        std::vector<OpenGLTimelineSubmit> waited_timelines_{};
        std::vector<OpenGLTimelineSubmit> signalled_timelines_{};
        std::vector<std::pair<resources::FenceNativeHandle, std::uint64_t>> signalled_fences_{};
        unsigned int shader_program_{0};
        int model_uniform_location_{-1};
        int view_uniform_location_{-1};
        int projection_uniform_location_{-1};
        int light_pos_uniform_location_{-1};
        int view_pos_uniform_location_{-1};
        int object_color_uniform_location_{-1};
        int has_texture_uniform_location_{-1};
        int texture_sampler_uniform_location_{-1};
        unsigned int default_texture_{0};
        bool default_texture_initialised_{false};

        void execute_draw_command(const GeometryDrawCommand& command);
        void execute_mesh_draw(const GeometryDrawCommand& command);
        void execute_point_cloud_draw(const GeometryDrawCommand& command);
        void execute_graph_draw(const GeometryDrawCommand& command);
        void execute_compute_dispatch(const ComputeDispatchCommand& command);
        bool ensure_shader_program();
        unsigned int compile_shader(unsigned int type, const char* source);
        void upload_matrix(int location, const engine::math::mat4& matrix);
        void upload_draw_uniforms(const GeometryDrawCommand& command, bool has_texture_coordinates);
        void bind_default_texture();
    };
}
