#include "engine/rendering/backend/opengl/runtime_adapter.hpp"

#include <utility>

namespace engine::rendering::backend::opengl
{
    OpenGLRuntimeSubmission::OpenGLRuntimeSubmission(MeshResolver mesh_resolver)
        : render_resources(std::move(mesh_resolver))
        , command_stream(render_resources)
        , device_resources()
        , command_encoders(device_resources)
        , scheduler(device_resources, &command_stream)
    {
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
