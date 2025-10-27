#include "engine/rendering/backend/opengl/gpu_scheduler.hpp"

#ifndef ENGINE_RENDERING_HAS_GLAD
#    define ENGINE_RENDERING_HAS_GLAD 0
#endif

#if ENGINE_RENDERING_HAS_GLAD
#    include <glad/gl.h>
#endif

namespace engine::rendering::backend::opengl
{
    namespace
    {
        class DefaultCommandStream final : public CommandStream
        {
        public:
            void begin_submission(const OpenGLSubmission&) override
            {
            }

            void wait_timeline(const OpenGLTimelineSubmit&) override
            {
            }

            void issue_memory_barrier(std::uint32_t mask) override
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

            void execute_command_buffer(const OpenGLCommandEncoderSubmit&) override
            {
            }

            void signal_timeline(const OpenGLTimelineSubmit&) override
            {
            }

            void signal_fence(resources::FenceNativeHandle, std::uint64_t) override
            {
            }

            void end_submission(const OpenGLSubmission&) override
            {
#if ENGINE_RENDERING_HAS_GLAD
                if (glad_glFlush != nullptr)
                {
                    glad_glFlush();
                }
#endif
            }
        };
    } // namespace

    CommandStream& default_command_stream() noexcept
    {
        static DefaultCommandStream stream{};
        return stream;
    }
}