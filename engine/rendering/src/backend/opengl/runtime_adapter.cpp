#include "engine/rendering/backend/opengl/runtime_adapter.hpp"

#include <utility>

namespace engine::rendering::backend::opengl
{
    OpenGLRuntimeSubmission::OpenGLRuntimeSubmission(MeshResolver mesh_resolver,
                                                     std::uint64_t retention_frames)
        : render_resources(std::move(mesh_resolver))
        , command_stream(render_resources)
        , device_resources(retention_frames)
        , command_encoders(device_resources)
        , scheduler(device_resources, &command_stream)
    {
    }

    void OpenGLRuntimeSubmission::set_retention_frames(std::uint64_t frames) noexcept
    {
        device_resources.set_retention_frames(frames);
    }

    std::uint64_t OpenGLRuntimeSubmission::retention_frames() const noexcept
    {
        return device_resources.retention_frames();
    }

    RuntimeSubmissionContext OpenGLRuntimeSubmission::make_context(MaterialSystem& materials,
                                                                   FrameGraph& frame_graph,
                                                                   ForwardPipeline* pipeline) noexcept
    {
        return RuntimeSubmissionContext{
            render_resources,
            materials,
            device_resources,
            scheduler,
            command_encoders,
            frame_graph,
            pipeline,
        };
    }
} // namespace engine::rendering::backend::opengl
