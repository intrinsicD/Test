/**
 * @file prototype_viewer.cpp
 * @brief Interactive 3D model viewer prototype with drag-and-drop and camera controls
 *
 * This prototype application serves as a visual testing ground for the rendering engine.
 * Features:
 * - Drag-and-drop 3D model loading
 * - Mouse-controlled trackball camera (rotation around object)
 * - Mouse wheel zoom
 * - ImGui UI for feedback and controls
 * - Integration with rendering pipeline
 */

#include "engine/platform/windowing/window.hpp"
#include "engine/platform/input/input_state.hpp"
#include "engine/rendering/api.hpp"
#include "engine/rendering/frame_graph.hpp"
#include "engine/rendering/pipeline/research_baseline.hpp"
#include "engine/rendering/backend/opengl/gpu_scheduler.hpp"
#include "engine/rendering/backend/opengl/resource_provider.hpp"
#include "engine/scene/scene.hpp"
#include "engine/scene/components/transform.hpp"
#include "engine/rendering/components.hpp"
#include "engine/math/transform.hpp"
#include "engine/math/matrix.hpp"
#include "engine/math/quaternion.hpp"
#include "engine/math/vector.hpp"
#include "engine/tools/imgui_helpers.hpp"

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <glad/gl.h>
#include <GLFW/glfw3.h>

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

namespace
{
    /// Camera state for trackball-style orbit controls
    struct CameraState
    {
        engine::math::vec3 target{0.0F, 0.0F, 0.0F};     // Look-at point
        float distance{5.0F};                            // Distance from target
        float yaw{0.0F};                                 // Horizontal rotation (radians)
        float pitch{0.3F};                               // Vertical rotation (radians)
        
        // Mouse interaction state
        bool is_rotating{false};
        engine::math::vec2 last_mouse_pos{0.0F, 0.0F};
        
        /// Get the camera position in world space
        engine::math::vec3 position() const
        {
            const float cos_pitch = std::cos(pitch);
            const float sin_pitch = std::sin(pitch);
            const float cos_yaw = std::cos(yaw);
            const float sin_yaw = std::sin(yaw);
            
            engine::math::vec3 offset{
                distance * cos_pitch * sin_yaw,
                distance * sin_pitch,
                distance * cos_pitch * cos_yaw
            };
            
            return target + offset;
        }
        
        /// Get the view matrix for rendering
        engine::math::mat4 view_matrix() const
        {
            const auto pos = position();
            const engine::math::vec3 up{0.0F, 1.0F, 0.0F};
            
            // Look-at matrix construction
            engine::math::vec3 forward = target - pos;
            const float length = std::sqrt(forward[0] * forward[0] + forward[1] * forward[1] + forward[2] * forward[2]);
            if (length > 1e-6F)
            {
                forward = forward / length;
            }
            
            engine::math::vec3 right{
                forward[1] * up[2] - forward[2] * up[1],
                forward[2] * up[0] - forward[0] * up[2],
                forward[0] * up[1] - forward[1] * up[0]
            };
            const float right_length = std::sqrt(right[0] * right[0] + right[1] * right[1] + right[2] * right[2]);
            if (right_length > 1e-6F)
            {
                right = right / right_length;
            }
            
            engine::math::vec3 camera_up{
                right[1] * forward[2] - right[2] * forward[1],
                right[2] * forward[0] - right[0] * forward[2],
                right[0] * forward[1] - right[1] * forward[0]
            };
            
            engine::math::mat4 view = engine::math::identity_matrix<float, 4>();
            view(0, 0) = right[0];    view(0, 1) = right[1];    view(0, 2) = right[2];
            view(1, 0) = camera_up[0]; view(1, 1) = camera_up[1]; view(1, 2) = camera_up[2];
            view(2, 0) = -forward[0]; view(2, 1) = -forward[1]; view(2, 2) = -forward[2];
            view(0, 3) = -(right[0] * pos[0] + right[1] * pos[1] + right[2] * pos[2]);
            view(1, 3) = -(camera_up[0] * pos[0] + camera_up[1] * pos[1] + camera_up[2] * pos[2]);
            view(2, 3) = forward[0] * pos[0] + forward[1] * pos[1] + forward[2] * pos[2];
            
            return view;
        }
    };
    
    /// Application state
    struct ApplicationState
    {
        CameraState camera;
        std::vector<std::string> loaded_files;
        std::string status_message{"Ready. Drag and drop a 3D model file to load."};
        bool show_ui{true};
        int viewport_width{1280};
        int viewport_height{720};
    };
    
    ApplicationState g_app_state;
    
    /// GLFW drop callback for drag-and-drop file loading
    void drop_callback(GLFWwindow* window, int count, const char** paths)
    {
        (void)window;
        
        g_app_state.loaded_files.clear();
        for (int i = 0; i < count; ++i)
        {
            g_app_state.loaded_files.push_back(paths[i]);
        }
        
        if (count > 0)
        {
            g_app_state.status_message = "Loaded " + std::to_string(count) + " file(s): " + paths[0];
            if (count > 1)
            {
                g_app_state.status_message += " + " + std::to_string(count - 1) + " more";
            }
        }
    }
    
    /// GLFW scroll callback for zoom
    void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
    {
        (void)window;
        (void)xoffset;
        
        // ImGui might capture the scroll
        ImGuiIO& io = ImGui::GetIO();
        if (io.WantCaptureMouse)
        {
            return;
        }
        
        // Zoom by adjusting camera distance
        const float zoom_speed = 0.5F;
        g_app_state.camera.distance -= static_cast<float>(yoffset) * zoom_speed;
        g_app_state.camera.distance = std::max(0.5F, std::min(g_app_state.camera.distance, 50.0F));
    }
    
    /// Update camera based on mouse input
    void update_camera_input(GLFWwindow* window)
    {
        ImGuiIO& io = ImGui::GetIO();
        
        // Don't process camera input if ImGui wants the mouse
        if (io.WantCaptureMouse)
        {
            g_app_state.camera.is_rotating = false;
            return;
        }
        
        // Check for mouse button press/release
        const bool mouse_down = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
        
        double xpos = 0.0;
        double ypos = 0.0;
        glfwGetCursorPos(window, &xpos, &ypos);
        
        const engine::math::vec2 current_mouse{static_cast<float>(xpos), static_cast<float>(ypos)};
        
        if (mouse_down)
        {
            if (g_app_state.camera.is_rotating)
            {
                // Calculate mouse delta
                const engine::math::vec2 delta = current_mouse - g_app_state.camera.last_mouse_pos;
                
                // Update camera angles (trackball rotation)
                const float rotation_speed = 0.005F;
                g_app_state.camera.yaw += delta[0] * rotation_speed;
                g_app_state.camera.pitch -= delta[1] * rotation_speed;
                
                // Clamp pitch to avoid gimbal lock
                const float pi = 3.14159265359F;
                g_app_state.camera.pitch = std::max(-pi * 0.49F, std::min(g_app_state.camera.pitch, pi * 0.49F));
            }
            else
            {
                g_app_state.camera.is_rotating = true;
            }
        }
        else
        {
            g_app_state.camera.is_rotating = false;
        }
        
        g_app_state.camera.last_mouse_pos = current_mouse;
    }
    
    /// Render ImGui UI
    void render_ui()
    {
        if (!g_app_state.show_ui)
        {
            return;
        }
        
        ImGui::Begin("Prototype Viewer", &g_app_state.show_ui);
        
        ImGui::TextWrapped("%s", g_app_state.status_message.c_str());
        ImGui::Separator();
        
        ImGui::Text("Camera Controls:");
        ImGui::BulletText("Left Mouse + Drag: Rotate camera (trackball)");
        ImGui::BulletText("Mouse Wheel: Zoom in/out");
        ImGui::BulletText("Drag & Drop: Load 3D model files");
        
        ImGui::Separator();
        ImGui::Text("Camera State:");
        ImGui::Text("Distance: %.2f", g_app_state.camera.distance);
        ImGui::Text("Yaw: %.2f°", g_app_state.camera.yaw * 180.0F / 3.14159265359F);
        ImGui::Text("Pitch: %.2f°", g_app_state.camera.pitch * 180.0F / 3.14159265359F);
        
        const auto pos = g_app_state.camera.position();
        ImGui::Text("Position: (%.2f, %.2f, %.2f)", pos[0], pos[1], pos[2]);
        
        if (ImGui::Button("Reset Camera"))
        {
            g_app_state.camera = CameraState{};
        }
        
        ImGui::Separator();
        if (!g_app_state.loaded_files.empty())
        {
            ImGui::Text("Loaded Files:");
            for (const auto& file : g_app_state.loaded_files)
            {
                ImGui::BulletText("%s", file.c_str());
            }
        }
        else
        {
            ImGui::TextDisabled("No files loaded");
        }
        
        ImGui::End();
    }
    
} // anonymous namespace

int main(int argc, char* argv[])
{
    (void)argc;
    (void)argv;
    
    std::cout << "=== Test Engine Prototype Viewer ===\n";
    std::cout << "Interactive 3D Model Viewer with Drag-and-Drop\n\n";
    
    // Initialize GLFW
    if (!glfwInit())
    {
        std::cerr << "Failed to initialize GLFW\n";
        return EXIT_FAILURE;
    }
    
    // Set OpenGL version hints
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    #ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    #endif
    
    // Create window
    GLFWwindow* window = glfwCreateWindow(
        g_app_state.viewport_width, 
        g_app_state.viewport_height,
        "Test Engine Prototype Viewer",
        nullptr,
        nullptr
    );
    
    if (!window)
    {
        std::cerr << "Failed to create GLFW window\n";
        glfwTerminate();
        return EXIT_FAILURE;
    }
    
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // Enable vsync
    
    // Initialize GLAD
    if (!gladLoadGL(glfwGetProcAddress))
    {
        std::cerr << "Failed to initialize GLAD\n";
        glfwDestroyWindow(window);
        glfwTerminate();
        return EXIT_FAILURE;
    }
    
    std::cout << "OpenGL version: " << glGetString(GL_VERSION) << "\n";
    
    // Set up callbacks
    glfwSetDropCallback(window, drop_callback);
    glfwSetScrollCallback(window, scroll_callback);
    
    // Initialize ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    
    ImGui::StyleColorsDark();
    
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330 core");
    
    std::cout << "Window created. Starting main loop...\n";
    std::cout << "\nControls:\n";
    std::cout << "  - Left Mouse + Drag: Rotate camera (trackball)\n";
    std::cout << "  - Mouse Wheel: Zoom in/out\n";
    std::cout << "  - Drag & Drop: Load 3D model files\n";
    std::cout << "  - ESC: Close window\n\n";
    
    // Main loop
    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();
        
        // Handle escape key
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        {
            glfwSetWindowShouldClose(window, GLFW_TRUE);
        }
        
        // Update camera based on input
        update_camera_input(window);
        
        // Get current framebuffer size
        int display_w = 0;
        int display_h = 0;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        g_app_state.viewport_width = display_w;
        g_app_state.viewport_height = display_h;
        
        // Start ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        
        // Render UI
        render_ui();
        
        // Render
        glViewport(0, 0, display_w, display_h);
        glClearColor(0.1F, 0.1F, 0.12F, 1.0F);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        // TODO: Render 3D scene here using the rendering pipeline
        // This would involve:
        // - Setting up the frame graph with the camera view matrix
        // - Executing the rendering passes
        // - Displaying loaded models
        
        // Render ImGui
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        
        glfwSwapBuffers(window);
    }
    
    // Cleanup
    std::cout << "Shutting down...\n";
    
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    
    glfwDestroyWindow(window);
    glfwTerminate();
    
    std::cout << "✓ Prototype viewer closed successfully\n";
    
    return EXIT_SUCCESS;
}
