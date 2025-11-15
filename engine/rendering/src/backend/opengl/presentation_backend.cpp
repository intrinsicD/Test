#include "engine/core/log.hpp"
#include "engine/scene/scene.hpp"
#include "engine/rendering/backend/opengl/presentation_backend.hpp"

#include <stdexcept>
#include <string>
#include <utility>

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
                                                         std::uint64_t retention_frames)
        : submission_(std::move(mesh_resolver), std::move(point_cloud_resolver), retention_frames)
        , pipeline_(std::move(pipeline))
    {
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
            ENGINE_ERROR("RuntimePresentationContext.submit_render_graph must be set before presentation");
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
        // If context already initialized for this window, nothing to do
        if (context_initialized_ && current_window_ == window_handle)
        {
            return;
        }

        auto* window = static_cast<GLFWwindow*>(window_handle);

        // Make the OpenGL context current
        glfwMakeContextCurrent(window);

        ENGINE_INFO("OpenGL Presentation Backend: Initializing context");

        // Load OpenGL functions on first initialization
        if (!context_initialized_)
        {
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

        // Set viewport to match framebuffer size
        int width, height;
        glfwGetFramebufferSize(window, &width, &height);
#    if ENGINE_RENDERING_HAS_GLAD
        glViewport(0, 0, width, height);
#    endif
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

