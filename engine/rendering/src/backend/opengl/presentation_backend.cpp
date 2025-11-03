#include "engine/rendering/backend/opengl/presentation_backend.hpp"

#include <stdexcept>
#include <utility>

namespace engine::rendering::backend::opengl
{
    OpenGLPresentationBackend::OpenGLPresentationBackend(MeshResolver mesh_resolver,
                                                         std::unique_ptr<ForwardPipeline> pipeline,
                                                         std::uint64_t retention_frames)
        : submission_(std::move(mesh_resolver), retention_frames)
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

    void OpenGLPresentationBackend::set_resource_retention_frames(std::uint64_t frames) noexcept
    {
        submission_.set_retention_frames(frames);
    }

    std::uint64_t OpenGLPresentationBackend::resource_retention_frames() const noexcept
    {
        return submission_.retention_frames();
    }
} // namespace engine::rendering::backend::opengl

