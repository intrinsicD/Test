#include "engine/core/log.hpp"
#include "engine/scene/scene.hpp"
#include "engine/rendering/backend/opengl/presentation_backend.hpp"

#include <stdexcept>
#include <string>
#include <utility>

// Add ImGui backend includes when GLFW/GLAD are enabled
#if ENGINE_PLATFORM_HAS_GLFW && ENGINE_RENDERING_HAS_GLAD
#include <imgui.h>
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"
#endif

// OpenGL headers for context management and rendering
#ifndef ENGINE_RENDERING_HAS_GLAD
#    define ENGINE_RENDERING_HAS_GLAD 0
#endif

#ifndef ENGINE_PLATFORM_HAS_GLFW
#    define ENGINE_PLATFORM_HAS_GLFW 0
#endif

#if ENGINE_RENDERING_HAS_GLAD
#    include <glad/gl.h>
#endif

#if ENGINE_PLATFORM_HAS_GLFW
// GLFW for window handle and buffer swap
#    define GLFW_INCLUDE_NONE
#    include <GLFW/glfw3.h>
#endif

namespace engine::rendering::backend::opengl
{
    OpenGLPresentationBackend::OpenGLPresentationBackend(MeshResolver mesh_resolver,
                                                         PointCloudResolver point_cloud_resolver,
                                                         std::unique_ptr<ForwardPipeline> pipeline,
                                                         std::uint64_t retention_frames,
                                                         GraphResolver graph_resolver)
        : submission_(std::move(mesh_resolver),
                      std::move(point_cloud_resolver),
                      retention_frames,
                      std::move(graph_resolver))
        , pipeline_(std::move(pipeline))
    {
    }

    OpenGLPresentationBackend::~OpenGLPresentationBackend() noexcept
    {
#if ENGINE_PLATFORM_HAS_GLFW && ENGINE_RENDERING_HAS_GLAD
        // If backends were initialized, attempt to shut them down using the registered ImGui context
        if (imgui_backend_initialized_)
        {
            ImGuiContext* prev = ImGui::GetCurrentContext();
            if (imgui_context_for_rendering_ != nullptr)
            {
                ImGui::SetCurrentContext(imgui_context_for_rendering_);
            }

            // Renderer first
            if (ImGui::GetIO().BackendRendererUserData != nullptr)
            {
                ImGui_ImplOpenGL3_Shutdown();
            }

            // Then platform
            if (ImGui::GetIO().BackendPlatformUserData != nullptr)
            {
                ImGui_ImplGlfw_Shutdown();
            }

            // Clear any lingering userdata to avoid other shutdown assertions
            ImGui::GetIO().BackendRendererUserData = nullptr;
            ImGui::GetIO().BackendPlatformUserData = nullptr;
            ImGui::GetIO().BackendPlatformName = nullptr;

            // Restore previous context
            ImGui::SetCurrentContext(prev);
        }
#endif
    }

    void OpenGLPresentationBackend::present(const RuntimePresentationContext& context)
    {
#if ENGINE_PLATFORM_HAS_GLFW
        auto* window = static_cast<GLFWwindow*>(context.native_window_handle);

        // Only perform window operations if a window is available
        if (window)
        {
            // Initialize OpenGL context on first use or window change
            initialize_context_if_needed(window);

            // Clear the framebuffer
            clear_framebuffer();
        }
#else
        if (context.native_window_handle != nullptr)
        {
            ENGINE_WARN("OpenGL presentation backend running without GLFW; native window handle ignored.");
        }
#endif

        // Execute frame graph (submits draw commands) even without a window for testing
        auto* pipeline = pipeline_.get();
        auto submission_context = submission_.make_context(material_system(), frame_graph(), pipeline);
        const auto submit_render_graph = context.submit_render_graph;
        if (submit_render_graph == nullptr)
        {
            ENGINE_ERROR(
                "RuntimePresentationContext.submit_render_graph must be set before presentation; skipping frame submission.");
            return;
        }
        submit_render_graph(context.host, submission_context);

        // Swap buffers to present rendered frame (only if window available)
#if ENGINE_PLATFORM_HAS_GLFW
        if (window)
        {
            swap_buffers(window);
        }
#endif
    }

    void OpenGLPresentationBackend::present_with_scene(scene::Scene& scene, void* window_handle)
    {
        ENGINE_INFO("OpenGL Backend: present_with_scene called");

#if ENGINE_PLATFORM_HAS_GLFW
        auto* window = static_cast<GLFWwindow*>(window_handle);
        if (!window)
        {
            ENGINE_WARN("  No window handle provided");
            return;
        }

        // Initialize OpenGL context on first use or window change
        initialize_context_if_needed(window);

        // Clear the framebuffer
        clear_framebuffer();

        // Create submission context with OpenGL providers
        auto submission_context = submission_.make_context(materials_, frame_graph_, nullptr);

        // Create execution context with the provided scene
        auto execution_context = submission_context.make_execution_context(scene);

        ENGINE_INFO("  Executing frame graph with {} entities", scene.size());

        // Execute frame graph
        frame_graph_.execute(execution_context);

        // If the application registered an ImGui context for rendering, render its draw data here
        // while the OpenGL context is current.
#if ENGINE_PLATFORM_HAS_GLFW && ENGINE_RENDERING_HAS_GLAD
        if (imgui_context_for_rendering_ != nullptr && imgui_backend_initialized_)
        {
            ImGuiContext* previous_ctx = ImGui::GetCurrentContext();
            ImGui::SetCurrentContext(imgui_context_for_rendering_);

            if (ImGui::GetDrawData())
            {
                ImDrawData* dd = ImGui::GetDrawData();
                ENGINE_INFO("ImGui draw data: CmdLists={} TotalVtx={} TotalIdx={}", dd->CmdListsCount, dd->TotalVtxCount, dd->TotalIdxCount);
                ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
            }

            // Handle multi-viewport platform windows if enabled
            ImGuiIO& io = ImGui::GetIO();
            if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
            {
                ImGui::UpdatePlatformWindows();
                ImGui::RenderPlatformWindowsDefault();
            }

            ImGui::SetCurrentContext(previous_ctx);
        }
#endif

        // If the application requested an ImGui render, call the registered callback while the GL context is current.
#if ENGINE_PLATFORM_HAS_GLFW && ENGINE_RENDERING_HAS_GLAD
        if (imgui_render_requested_ && imgui_render_callback_ && imgui_context_for_rendering_)
        {
            ImGuiContext* prev_ctx = ImGui::GetCurrentContext();
            ImGui::SetCurrentContext(imgui_context_for_rendering_);

            // Ensure backends are initialized before invoking the callback (will have been initialized in initialize_context_if_needed)
            imgui_render_callback_(imgui_last_delta_);

            // Now render draw data
            if (ImGui::GetDrawData())
            {
                ImDrawData* dd = ImGui::GetDrawData();
                ENGINE_INFO("ImGui draw data (app-callback): CmdLists={} TotalVtx={} TotalIdx={}", dd->CmdListsCount, dd->TotalVtxCount, dd->TotalIdxCount);
                ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
            }

            // Multi-viewport handling if enabled
            ImGuiIO& io = ImGui::GetIO();
            if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
            {
                ImGui::UpdatePlatformWindows();
                ImGui::RenderPlatformWindowsDefault();
            }

            ImGui::SetCurrentContext(prev_ctx);
            imgui_render_requested_ = false;
        }
#endif

        // Swap buffers to present rendered frame
        swap_buffers(window);
#else
        (void)scene;
        (void)window_handle;
        ENGINE_WARN("OpenGL presentation backend running without GLFW; present_with_scene is a no-op.");
#endif
    }

    void OpenGLPresentationBackend::set_resource_retention_frames(std::uint64_t frames) noexcept
    {
        submission_.set_retention_frames(frames);
    }

    std::uint64_t OpenGLPresentationBackend::resource_retention_frames() const noexcept
    {
        return submission_.retention_frames();
    }

    void OpenGLPresentationBackend::initialize_context_if_needed(void* window_handle)
    {
#if ENGINE_PLATFORM_HAS_GLFW
        if (window_handle == nullptr)
        {
            ENGINE_WARN("OpenGL presentation backend received a null window handle; skipping context initialization.");
            return;
        }

        auto* window = static_cast<GLFWwindow*>(window_handle);

        // Ensure the OpenGL context for this window is current before issuing any GL calls.
        glfwMakeContextCurrent(window);

        // Load OpenGL functions and configure state on first initialization.
        if (!context_initialized_)
        {
            ENGINE_INFO("OpenGL Presentation Backend: Initializing context");
#    if ENGINE_RENDERING_HAS_GLAD
            int version = gladLoadGL(reinterpret_cast<GLADloadfunc>(glfwGetProcAddress));
            if (version == 0)
            {
                throw std::runtime_error("Failed to load OpenGL functions with GLAD");
            }

            ENGINE_INFO("  ✓ GLAD loaded OpenGL {}.{}", GLAD_VERSION_MAJOR(version), GLAD_VERSION_MINOR(version));

            // Enable depth testing
            glEnable(GL_DEPTH_TEST);
            glDepthFunc(GL_LESS);

            // Enable backface culling
            glEnable(GL_CULL_FACE);
            glCullFace(GL_BACK);
            glFrontFace(GL_CCW);

            ENGINE_INFO("  ✓ OpenGL state configured (depth test, culling)");
#    else
            ENGINE_WARN("  ✗ GLAD not available - OpenGL functions will not work!");
#    endif
            context_initialized_ = true;
        }

        current_window_ = window_handle;

        // Set viewport to match framebuffer size on every frame so resizing is reflected immediately.
        int width = 0;
        int height = 0;
        glfwGetFramebufferSize(window, &width, &height);
        if (width <= 0)
        {
            width = 1;
        }
        if (height <= 0)
        {
            height = 1;
        }
#    if ENGINE_RENDERING_HAS_GLAD
        glViewport(0, 0, width, height);
#    endif

        // Initialize ImGui platform/renderer backends if an ImGui context exists and backends aren't initialized.
        // This allows ImGui_ImplGlfw_NewFrame() to populate io.DisplaySize before ImGui::NewFrame() is called.
#if ENGINE_PLATFORM_HAS_GLFW && ENGINE_RENDERING_HAS_GLAD
        if (!imgui_backend_initialized_)
        {
            // Prefer to initialize backends using the ImGui context the application registered
            // so that backend userdata is correctly associated and shutdown will work.
            ImGuiContext* prev_ctx = ImGui::GetCurrentContext();
            if (imgui_context_for_rendering_ != nullptr)
            {
                ImGui::SetCurrentContext(imgui_context_for_rendering_);
            }

            // Initialize backends without installing GLFW callbacks (application manages input forwarding)
            ImGui_ImplGlfw_InitForOpenGL(window, /*install_callbacks=*/true);
            ImGui_ImplOpenGL3_Init();
            imgui_backend_initialized_ = true;
            ENGINE_INFO("  ✓ ImGui GLFW/OpenGL3 backends initialized (registered context)");

            // Restore previous context
            ImGui::SetCurrentContext(prev_ctx);
        }

        // Update the ImGui IO display size every frame so layout responds to window resizing.
        if (imgui_context_for_rendering_ != nullptr || ImGui::GetCurrentContext() != nullptr)
        {
            ImGuiContext* save_ctx = ImGui::GetCurrentContext();
            if (imgui_context_for_rendering_ != nullptr)
            {
                ImGui::SetCurrentContext(imgui_context_for_rendering_);
            }
            ImGuiIO& io = ImGui::GetIO();
            io.DisplaySize = ImVec2(static_cast<float>(width), static_cast<float>(height));
            ImGui::SetCurrentContext(save_ctx);
        }
#endif
#else
        (void)window_handle;
#endif
    }

    void OpenGLPresentationBackend::clear_framebuffer()
    {
#if ENGINE_RENDERING_HAS_GLAD
        // Clear to a nice blue-grey color
        glClearColor(0.2f, 0.3f, 0.4f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
#endif
    }

    void OpenGLPresentationBackend::swap_buffers(void* window_handle)
    {
#if ENGINE_PLATFORM_HAS_GLFW
        auto* window = static_cast<GLFWwindow*>(window_handle);
        glfwSwapBuffers(window);
#else
        (void)window_handle;
#endif
    }
} // namespace engine::rendering::backend::opengl
