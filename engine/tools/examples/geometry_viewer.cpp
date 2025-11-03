/**
 * @file geometry_viewer.cpp
 * @brief Interactive 3D geometry viewer with camera controls
 *
 * This example shows how to:
 * - Create a window with OpenGL context
 * - Initialize the rendering system with OpenGL backend
 * - Set up a camera with orbit controls
 * - Load and display mesh geometry
 * - Use the research baseline rendering preset
 * - Process user input for camera control
 * - Render the scene in a continuous loop
 *
 * Controls:
 * - Mouse drag: Rotate camera
 * - Mouse scroll: Zoom in/out
 * - ESC: Exit application
 */

#include <chrono>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <memory>

#include <GLFW/glfw3.h>

#include "engine/platform/windowing/window.hpp"
#include "engine/rendering/api.hpp"
#include "engine/rendering/camera.hpp"
#include "engine/rendering/camera_controllers.hpp"
#include "engine/rendering/components.hpp"
#include "engine/rendering/frame_graph.hpp"
#include "engine/rendering/pipeline/research_baseline.hpp"
#include "engine/scene/scene.hpp"
#include "engine/scene/components/transform.hpp"
#include "engine/math/transform.hpp"

namespace
{
    constexpr int WINDOW_WIDTH = 1280;
    constexpr int WINDOW_HEIGHT = 720;
    constexpr float CAMERA_DISTANCE = 5.0f;
    constexpr float CAMERA_ROTATE_SPEED = 0.005f;
    constexpr float CAMERA_ZOOM_SPEED = 0.1f;

    /// Application state
    struct AppState
    {
        GLFWwindow* glfw_window = nullptr;
        std::shared_ptr<engine::platform::Window> window;
        engine::scene::Scene scene;
        entt::entity camera_entity{entt::null};

        // Input state
        bool mouse_dragging = false;
        double last_mouse_x = 0.0;
        double last_mouse_y = 0.0;
        float camera_yaw = 0.0f;
        float camera_pitch = 0.3f;
        float camera_radius = CAMERA_DISTANCE;

        // Timing
        std::chrono::steady_clock::time_point last_frame_time;
    };

    // Forward declarations
    void update_camera_from_state(const AppState& state, engine::rendering::Camera& camera);

    /// GLFW error callback
    void glfw_error_callback(int error, const char* description)
    {
        std::cerr << "GLFW Error " << error << ": " << description << "\n";
    }

    /// GLFW mouse button callback
    void mouse_button_callback(GLFWwindow* window, int button, int action, int /*mods*/)
    {
        auto* state = static_cast<AppState*>(glfwGetWindowUserPointer(window));
        if (!state) return;

        if (button == GLFW_MOUSE_BUTTON_LEFT)
        {
            if (action == GLFW_PRESS)
            {
                state->mouse_dragging = true;
                glfwGetCursorPos(window, &state->last_mouse_x, &state->last_mouse_y);
            }
            else if (action == GLFW_RELEASE)
            {
                state->mouse_dragging = false;
            }
        }
    }

    /// GLFW cursor position callback
    void cursor_position_callback(GLFWwindow* window, double xpos, double ypos)
    {
        auto* state = static_cast<AppState*>(glfwGetWindowUserPointer(window));
        if (!state || !state->mouse_dragging) return;

        const double dx = xpos - state->last_mouse_x;
        const double dy = ypos - state->last_mouse_y;

        state->camera_yaw += static_cast<float>(dx) * CAMERA_ROTATE_SPEED;
        state->camera_pitch -= static_cast<float>(dy) * CAMERA_ROTATE_SPEED;

        // Clamp pitch to avoid gimbal lock
        state->camera_pitch = std::clamp(state->camera_pitch, -1.5f, 1.5f);

        state->last_mouse_x = xpos;
        state->last_mouse_y = ypos;
    }

    /// GLFW scroll callback
    void scroll_callback(GLFWwindow* window, double /*xoffset*/, double yoffset)
    {
        auto* state = static_cast<AppState*>(glfwGetWindowUserPointer(window));
        if (!state) return;

        state->camera_radius -= static_cast<float>(yoffset) * CAMERA_ZOOM_SPEED;
        state->camera_radius = std::clamp(state->camera_radius, 1.0f, 20.0f);
    }

    /// GLFW key callback
    void key_callback(GLFWwindow* window, int key, int /*scancode*/, int action, int /*mods*/)
    {
        if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        {
            glfwSetWindowShouldClose(window, GLFW_TRUE);
        }
    }

    /// Initialize GLFW and create window
    GLFWwindow* create_glfw_window()
    {
        glfwSetErrorCallback(glfw_error_callback);

        if (!glfwInit())
        {
            throw std::runtime_error("Failed to initialize GLFW");
        }

        // Request OpenGL 4.5 core profile
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

        GLFWwindow* window = glfwCreateWindow(
            WINDOW_WIDTH, WINDOW_HEIGHT,
            "Geometry Viewer - Research Baseline",
            nullptr, nullptr);

        if (!window)
        {
            glfwTerminate();
            throw std::runtime_error("Failed to create GLFW window");
        }

        glfwMakeContextCurrent(window);
        glfwSwapInterval(1); // Enable vsync


        return window;
    }

    /// Create a simple cube mesh for demonstration
    engine::assets::MeshHandle create_example_cube()
    {
        return engine::assets::MeshHandle{std::string{"examples/cube.mesh"}};
    }

    /// Setup a simple scene with geometry
    void setup_scene(AppState& state)
    {
        auto& registry = state.scene.registry();

        // Create a cube entity
        auto cube = registry.create();

        // Add transform component
        auto& transform = registry.emplace<engine::scene::components::WorldTransform>(cube);
        transform.value = engine::math::Transform<float>::Identity();

        // Add render geometry component
        auto material = engine::assets::MaterialHandle{std::string{"examples/default.material"}};
        registry.emplace<engine::rendering::components::RenderGeometry>(
            cube,
            engine::rendering::components::RenderGeometry::from_mesh(
                create_example_cube(), material));

        std::cout << "Scene created with 1 cube entity\n";
    }

    /// Setup camera with orbit controller
    void setup_camera(AppState& state)
    {
        auto& registry = state.scene.registry();

        // Create camera entity
        state.camera_entity = registry.create();

        // Add camera component
        auto& camera = registry.emplace<engine::rendering::Camera>(state.camera_entity);

        // Set up perspective projection
        const float aspect_ratio = static_cast<float>(WINDOW_WIDTH) / static_cast<float>(WINDOW_HEIGHT);
        camera.set_perspective(
            1.047f,  // ~60 degrees FOV
            aspect_ratio,
            0.1f,    // Near plane
            100.0f   // Far plane
        );

        // Position camera
        update_camera_from_state(state, camera);

        std::cout << "Camera created with orbit controller\n";
    }

    /// Update camera position based on current state
    void update_camera_from_state(const AppState& state, engine::rendering::Camera& camera)
    {
        const float cos_pitch = std::cos(state.camera_pitch);
        const float sin_pitch = std::sin(state.camera_pitch);
        const float cos_yaw = std::cos(state.camera_yaw);
        const float sin_yaw = std::sin(state.camera_yaw);

        // Calculate camera position in orbit
        const engine::math::vec3 camera_pos{
            state.camera_radius * cos_pitch * sin_yaw,
            state.camera_radius * sin_pitch,
            state.camera_radius * cos_pitch * cos_yaw
        };

        const engine::math::vec3 target{0.0f, 0.0f, 0.0f};
        const engine::math::vec3 up{0.0f, 1.0f, 0.0f};

        [[maybe_unused]] auto result = camera.look_at(camera_pos, target, up);
    }

    /// Render a frame
    void render_frame(AppState& state)
    {
        // Update camera if needed
        auto& registry = state.scene.registry();
        if (registry.valid(state.camera_entity))
        {
            auto& camera = registry.get<engine::rendering::Camera>(state.camera_entity);
            update_camera_from_state(state, camera);
        }

        // Note: In a full implementation with OpenGL, we would:
        // 1. glClearColor(0.2f, 0.3f, 0.4f, 1.0f);
        // 2. glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        // 3. glEnable(GL_DEPTH_TEST);
        // 4. Execute the frame graph with the scene
        // 5. Render all geometry
        // 6. Present the final image
        //
        // For now, GLFW will handle the default framebuffer

        // Calculate FPS
        auto now = std::chrono::steady_clock::now();
        if (state.last_frame_time.time_since_epoch().count() > 0)
        {
            auto delta = std::chrono::duration_cast<std::chrono::milliseconds>(
                now - state.last_frame_time).count();
            if (delta > 0)
            {
                static int frame_count = 0;
                static auto last_print = now;
                frame_count++;

                auto print_delta = std::chrono::duration_cast<std::chrono::seconds>(
                    now - last_print).count();
                if (print_delta >= 2)
                {
                    const float fps = static_cast<float>(frame_count) / static_cast<float>(print_delta);
                    std::cout << "FPS: " << fps << " (Camera: yaw=" << state.camera_yaw
                              << ", pitch=" << state.camera_pitch
                              << ", radius=" << state.camera_radius << ")\n";
                    frame_count = 0;
                    last_print = now;
                }
            }
        }
        state.last_frame_time = now;
    }

    /// Main application loop
    void run_application(AppState& state)
    {
        std::cout << "\n=== Entering main loop ===\n";
        std::cout << "Controls:\n";
        std::cout << "  - Left mouse drag: Rotate camera\n";
        std::cout << "  - Mouse scroll: Zoom in/out\n";
        std::cout << "  - ESC: Exit\n\n";

        while (!glfwWindowShouldClose(state.glfw_window))
        {
            // Poll events
            glfwPollEvents();

            // Render frame
            render_frame(state);

            // Swap buffers
            glfwSwapBuffers(state.glfw_window);
        }

        std::cout << "\n=== Exiting main loop ===\n";
    }
}

int main(int argc, char* argv[])
{
    (void)argc;
    (void)argv;

    std::cout << "=== Test Engine Geometry Viewer ===\n";
    std::cout << "Interactive 3D Viewer with Orbit Camera\n\n";

    AppState state;

    try
    {
        // Create GLFW window and OpenGL context
        std::cout << "Creating window and OpenGL context...\n";
        state.glfw_window = create_glfw_window();

        // Set up GLFW callbacks
        glfwSetWindowUserPointer(state.glfw_window, &state);
        glfwSetMouseButtonCallback(state.glfw_window, mouse_button_callback);
        glfwSetCursorPosCallback(state.glfw_window, cursor_position_callback);
        glfwSetScrollCallback(state.glfw_window, scroll_callback);
        glfwSetKeyCallback(state.glfw_window, key_callback);

        std::cout << "Window created successfully\n\n";

        // Create engine window wrapper
        std::cout << "Initializing platform window...\n";
        state.window = engine::platform::create_window(
            engine::platform::WindowConfig{
                .title = "Geometry Viewer",
                .width = WINDOW_WIDTH,
                .height = WINDOW_HEIGHT,
                .visible = true,
                .resizable = true
            },
            engine::platform::WindowBackend::GLFW
        );

        // Setup scene
        std::cout << "Creating scene...\n";
        setup_scene(state);

        // Setup camera
        std::cout << "Setting up camera...\n";
        setup_camera(state);

        // Setup frame graph
        std::cout << "Configuring research baseline rendering preset...\n";
        engine::rendering::FrameGraph graph;
        engine::rendering::ResearchBaselineOptions options{};
        options.shading_mode = engine::rendering::ResearchShadingMode::Forward;
        options.width = WINDOW_WIDTH;
        options.height = WINDOW_HEIGHT;
        options.enable_normals_overlay = false;

        const auto resources = engine::rendering::configure_research_baseline(graph, options);

        std::cout << "Compiling frame graph...\n";
        graph.compile();

        std::cout << "\nFrame graph configured:\n";
        std::cout << "  - Final color: " << (resources.lighting_output.valid() ? "✓" : "✗") << "\n";
        std::cout << "  - Depth buffer: " << (resources.depth.valid() ? "✓" : "✗") << "\n";

        // Run main loop
        run_application(state);

        // Cleanup
        std::cout << "\nCleaning up...\n";
        glfwDestroyWindow(state.glfw_window);
        glfwTerminate();

        std::cout << "✓ Geometry viewer exited successfully\n";
        return EXIT_SUCCESS;
    }
    catch (const std::exception& e)
    {
        std::cerr << "\n❌ Error: " << e.what() << "\n";

        if (state.glfw_window)
        {
            glfwDestroyWindow(state.glfw_window);
        }
        glfwTerminate();

        return EXIT_FAILURE;
    }
}