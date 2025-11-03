#pragma once

#include <memory>

#include "engine/rendering/backend/opengl/render_resource_provider.hpp"
#include "engine/rendering/backend/opengl/runtime_adapter.hpp"
#include "engine/rendering/frame_graph.hpp"
#include "engine/rendering/forward_pipeline.hpp"
#include "engine/rendering/material_system.hpp"
#include "engine/rendering/presentation_backend.hpp"

namespace engine::rendering::backend::opengl
{
    /**
     * \brief Presentation backend that executes the runtime frame graph using the OpenGL stack.
     *
     * The backend owns the resource providers, command encoder infrastructure, and
     * GPU scheduler required to execute the frame graph produced by the runtime
     * host. Callers provide a mesh resolver so render geometry handles can be
     * translated into CPU-resident meshes before draw commands are recorded.
     */
    class OpenGLPresentationBackend final : public PresentationBackend
    {
    public:
        using MeshResolver = OpenGLRenderResourceProvider::MeshResolver;

        explicit OpenGLPresentationBackend(MeshResolver mesh_resolver,
                                           std::unique_ptr<ForwardPipeline> pipeline = nullptr);

        [[nodiscard]] MaterialSystem& material_system() noexcept { return materials_; }
        [[nodiscard]] const MaterialSystem& material_system() const noexcept { return materials_; }

        [[nodiscard]] FrameGraph& frame_graph() noexcept { return frame_graph_; }
        [[nodiscard]] const FrameGraph& frame_graph() const noexcept { return frame_graph_; }

        [[nodiscard]] OpenGLRuntimeSubmission& submission() noexcept { return submission_; }
        [[nodiscard]] const OpenGLRuntimeSubmission& submission() const noexcept { return submission_; }

        void present(const RuntimePresentationContext& context) override;

    private:
        OpenGLRuntimeSubmission submission_;
        MaterialSystem materials_{};
        FrameGraph frame_graph_{};
        std::unique_ptr<ForwardPipeline> pipeline_{};
    };
} // namespace engine::rendering::backend::opengl

