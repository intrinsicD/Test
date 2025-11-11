#include "engine/rendering/backend/opengl/presentation_backend.hpp"

#include <stdexcept>
#include <utility>

// OpenGL headers for context management and rendering
#ifndef ENGINE_RENDERING_HAS_GLAD
#    define ENGINE_RENDERING_HAS_GLAD 0
#endif

#if ENGINE_RENDERING_HAS_GLAD
#    include <glad/gl.h>
#endif

// GLFW for window handle and buffer swap
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

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
        auto* window = static_cast<GLFWwindow*>(context.native_window_handle);
        if (!window)
        {
            // No window available, skip rendering
            return;
        }

        // Initialize OpenGL context on first use or window change
        initialize_context_if_needed(window);

        // Clear the framebuffer
        clear_framebuffer();

        // Execute frame graph (submits draw commands)
        auto* pipeline = pipeline_.get();
        auto submission_context = submission_.make_context(material_system(), frame_graph(), pipeline);
        const auto submit_render_graph = context.submit_render_graph;
        if (submit_render_graph == nullptr)
        {
            throw std::runtime_error("RuntimePresentationContext.submit_render_graph must be set before presentation");
        }
        submit_render_graph(context.host, submission_context);

        // Swap buffers to present rendered frame
        swap_buffers(window);
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
        // If context already initialized for this window, nothing to do
        if (context_initialized_ && current_window_ == window_handle)
        {
            return;
        }

        auto* window = static_cast<GLFWwindow*>(window_handle);

        // Make the OpenGL context current
        glfwMakeContextCurrent(window);

        // Load OpenGL functions on first initialization
        if (!context_initialized_)
        {
#if ENGINE_RENDERING_HAS_GLAD
            int version = gladLoadGL(reinterpret_cast<GLADloadfunc>(glfwGetProcAddress));
            if (version == 0)
            {
                throw std::runtime_error("Failed to load OpenGL functions with GLAD");
            }

            // Enable depth testing
            glEnable(GL_DEPTH_TEST);
            glDepthFunc(GL_LESS);

            // Enable backface culling
            glEnable(GL_CULL_FACE);
            glCullFace(GL_BACK);
            glFrontFace(GL_CCW);
#endif
            context_initialized_ = true;
        }

        current_window_ = window_handle;

        // Set viewport to match framebuffer size
        int width, height;
        glfwGetFramebufferSize(window, &width, &height);
#if ENGINE_RENDERING_HAS_GLAD
        glViewport(0, 0, width, height);
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
        auto* window = static_cast<GLFWwindow*>(window_handle);
        glfwSwapBuffers(window);
    }
} // namespace engine::rendering::backend::opengl

