#pragma once

#include <utility>

#include "engine/rendering/backend/opengl/command_encoder.hpp"
#include "engine/rendering/backend/opengl/gpu_scheduler.hpp"
#include "engine/rendering/backend/opengl/immediate_command_stream.hpp"
#include "engine/rendering/backend/opengl/render_resource_provider.hpp"
#include "engine/rendering/runtime_submission.hpp"

namespace engine::rendering::backend::opengl
{
    /**
     * \brief Convenience bundle wiring OpenGL runtime submission dependencies.
     *
     * The adapter owns an \ref OpenGLRenderResourceProvider, the immediate
     * command stream used to execute encoded draw calls, the OpenGL GPU resource
     * provider, the associated command encoder provider, and the GPU scheduler.
     * It exposes a helper for constructing \ref RuntimeSubmissionContext
     * instances backed by these components so runtime hosts or samples can
     * execute frame graphs without manually threading each dependency.
     */
    class OpenGLRuntimeSubmission
    {
    public:
        using MeshResolver = OpenGLRenderResourceProvider::MeshResolver;

        explicit OpenGLRuntimeSubmission(MeshResolver mesh_resolver);

        [[nodiscard]] RuntimeSubmissionContext make_context(MaterialSystem& materials,
                                                            FrameGraph& frame_graph,
                                                            ForwardPipeline* pipeline = nullptr) noexcept;

        OpenGLRenderResourceProvider render_resources;
        OpenGLImmediateCommandStream command_stream;
        OpenGLGpuResourceProvider device_resources;
        OpenGLCommandEncoderProvider command_encoders;
        OpenGLGpuScheduler scheduler;
    };
} // namespace engine::rendering::backend::opengl
