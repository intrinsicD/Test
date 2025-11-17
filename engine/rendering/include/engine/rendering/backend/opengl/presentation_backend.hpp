#pragma once

#include <cstdint>
#include <functional>
#include <memory>

#include "engine/rendering/backend/opengl/render_resource_provider.hpp"
#include "engine/rendering/backend/opengl/runtime_adapter.hpp"
#include "engine/rendering/frame_graph.hpp"
#include "engine/rendering/forward_pipeline.hpp"
#include "engine/rendering/material_system.hpp"
#include "engine/rendering/presentation_backend.hpp"

// Forward-declare ImGuiContext so this header doesn't need to pull in imgui.h
struct ImGuiContext;

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
        /// Register an ImGuiContext that will be used to render UI when presenting frames.
        void set_imgui_context_for_rendering(ImGuiContext* ctx) { imgui_context_for_rendering_ = ctx; }
        /// Request that the backend run the registered ImGui render callback on the next present.
        void request_imgui_render(double delta_time) { imgui_render_requested_ = true; imgui_last_delta_ = delta_time; }
        /// Provide a callback that will be invoked by the backend while the GL context and ImGui
        /// context are current; the callback should build ImGui windows (i.e. call panel_bridge_->render_all).
        void set_imgui_render_callback(std::function<void(double)> cb) { imgui_render_callback_ = std::move(cb); }

        using MeshResolver = OpenGLRenderResourceProvider::MeshResolver;
        using PointCloudResolver = OpenGLRenderResourceProvider::PointCloudResolver;

        explicit OpenGLPresentationBackend(MeshResolver mesh_resolver,
                                           PointCloudResolver point_cloud_resolver = {},
                                           std::unique_ptr<ForwardPipeline> pipeline = nullptr,
                                           std::uint64_t retention_frames = 0);
        ~OpenGLPresentationBackend() noexcept;

        [[nodiscard]] MaterialSystem& material_system() noexcept { return materials_; }
        [[nodiscard]] const MaterialSystem& material_system() const noexcept { return materials_; }

        [[nodiscard]] FrameGraph& frame_graph() noexcept { return frame_graph_; }
        [[nodiscard]] const FrameGraph& frame_graph() const noexcept { return frame_graph_; }

        [[nodiscard]] OpenGLRuntimeSubmission& submission() noexcept { return submission_; }
        [[nodiscard]] const OpenGLRuntimeSubmission& submission() const noexcept { return submission_; }

        void set_resource_retention_frames(std::uint64_t frames) noexcept;
        [[nodiscard]] std::uint64_t resource_retention_frames() const noexcept;

        void present(const RuntimePresentationContext& context) override;

        /// Present a frame using a custom scene (for applications that manage their own scene)
        void present_with_scene(scene::Scene& scene, void* window_handle);

    private:
        void initialize_context_if_needed(void* window_handle);
        void clear_framebuffer();
        void swap_buffers(void* window_handle);

        OpenGLRuntimeSubmission submission_;
        MaterialSystem materials_{};
        FrameGraph frame_graph_{};
        std::unique_ptr<ForwardPipeline> pipeline_{};

        bool context_initialized_{false};
        void* current_window_{nullptr};
        // Whether the ImGui GLFW/OpenGL backends have been initialized for the current process/context.
        bool imgui_backend_initialized_{false};
        // ImGui context pointer registered by the application for panel rendering.
        ImGuiContext* imgui_context_for_rendering_{nullptr};
        // Callback supplied by the application to build UI while the GL context is current.
        std::function<void(double)> imgui_render_callback_;
        // Request flag and last delta time passed from the application.
        bool imgui_render_requested_{false};
        double imgui_last_delta_{0.0};
    };
} // namespace engine::rendering::backend::opengl
