#include "engine/rendering/backend/opengl/presentation_backend.hpp"

#include <stdexcept>
#include <utility>

namespace engine::rendering::backend::opengl
{
    OpenGLPresentationBackend::OpenGLPresentationBackend(MeshResolver mesh_resolver,
                                                         std::unique_ptr<ForwardPipeline> pipeline)
        : submission_(std::move(mesh_resolver))
        , pipeline_(std::move(pipeline))
    {
    }

    void OpenGLPresentationBackend::present(const RuntimePresentationContext& context)
    {
        auto* pipeline = pipeline_.get();
        auto submission_context = submission_.make_context(material_system(), frame_graph(), pipeline);
        const auto submit_render_graph = context.submit_render_graph;
        if (submit_render_graph == nullptr)
        {
            throw std::runtime_error("RuntimePresentationContext.submit_render_graph must be set before presentation");
        }
        submit_render_graph(context.host, submission_context);
    }
} // namespace engine::rendering::backend::opengl

