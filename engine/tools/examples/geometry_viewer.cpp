/**
 * @file geometry_viewer.cpp
 * @brief Minimal example demonstrating geometry rendering with the research baseline preset
 *
 * This example shows how to:
 * - Initialize the rendering system with OpenGL backend
 * - Load and display mesh geometry
 * - Use the research baseline rendering preset
 * - Render geometry with the frame graph
 */

#include <cstdlib>
#include <iostream>
#include <memory>

#include "engine/rendering/api.hpp"
#include "engine/rendering/frame_graph.hpp"
#include "engine/rendering/pipeline/research_baseline.hpp"
#include "engine/rendering/backend/opengl/gpu_scheduler.hpp"
#include "engine/rendering/backend/opengl/resource_provider.hpp"
#include "engine/rendering/material_system.hpp"
#include "engine/scene/scene.hpp"
#include "engine/scene/components/transform.hpp"
#include "engine/rendering/components.hpp"
#include "engine/math/transform.hpp"

namespace
{
    /// Simple render resource provider for the example
    class ExampleResourceProvider final : public engine::rendering::RenderResourceProvider
    {
    public:
        void require_mesh(const engine::assets::MeshHandle& handle) override
        {
            if (!handle.empty())
            {
                std::cout << "Loading mesh: " << handle.id() << "\n";
            }
        }

        void require_graph(const engine::assets::GraphHandle& handle) override
        {
            if (!handle.empty())
            {
                std::cout << "Loading graph: " << handle.id() << "\n";
            }
        }

        void require_point_cloud(const engine::assets::PointCloudHandle& handle) override
        {
            if (!handle.empty())
            {
                std::cout << "Loading point cloud: " << handle.id() << "\n";
            }
        }

        void require_material(const engine::assets::MaterialHandle& handle) override
        {
            if (!handle.empty())
            {
                std::cout << "Loading material: " << handle.id() << "\n";
            }
        }

        void require_shader(const engine::assets::ShaderHandle& handle) override
        {
            if (!handle.empty())
            {
                std::cout << "Loading shader: " << handle.id() << "\n";
            }
        }
    };

    /// Create a simple cube mesh for demonstration
    engine::assets::MeshHandle create_example_cube()
    {
        // For now, return an empty handle - in a real application this would
        // create actual geometry data
        return engine::assets::MeshHandle{std::string{"examples/cube.mesh"}};
    }

    /// Setup a simple scene with a cube
    void setup_example_scene(engine::scene::Scene& scene)
    {
        auto& registry = scene.registry();

        // Create a cube entity
        auto cube = registry.create();

        // Add transform component
        auto& transform = registry.emplace<engine::scene::components::WorldTransform>(cube);
        transform.value = engine::math::Transform<float>::Identity();

        // Add render geometry component using the factory method
        auto material = engine::assets::MaterialHandle{std::string{"examples/default.material"}};
        registry.emplace<engine::rendering::components::RenderGeometry>(
            cube,
            engine::rendering::components::RenderGeometry::from_mesh(create_example_cube(), material));

        std::cout << "Scene created with 1 cube entity\n";
    }
}

int main(int argc, char* argv[])
{
    (void)argc;
    (void)argv;

    std::cout << "=== Test Engine Geometry Viewer ===\n";
    std::cout << "Research Baseline Rendering Example\n\n";

    try
    {
        // Initialize rendering backend
        std::cout << "Initializing OpenGL rendering backend...\n";
        auto resource_provider = std::make_unique<engine::rendering::backend::opengl::OpenGLGpuResourceProvider>();

        auto scheduler = std::make_unique<engine::rendering::backend::opengl::OpenGLGpuScheduler>(
            *resource_provider);

        // Create scene
        std::cout << "Creating scene...\n";
        engine::scene::Scene scene;
        setup_example_scene(scene);

        // Setup frame graph with research baseline preset
        std::cout << "Configuring research baseline rendering preset...\n";
        engine::rendering::FrameGraph graph;
        engine::rendering::ResearchBaselineOptions options{};
        options.shading_mode = engine::rendering::ResearchShadingMode::Forward;
        options.width = 1920;
        options.height = 1080;
        options.enable_normals_overlay = false;

        const auto resources = engine::rendering::configure_research_baseline(graph, options);

        std::cout << "Compiling frame graph...\n";
        graph.compile();

        std::cout << "\nFrame graph configured with resources:\n";
        std::cout << "  - Final color output: " << (resources.lighting_output.valid() ? "✓" : "✗") << "\n";
        std::cout << "  - Depth buffer: " << (resources.depth.valid() ? "✓" : "✗") << "\n";
        std::cout << "  - G-Buffer albedo: " << (resources.gbuffer_albedo.has_value() ? "✓" : "✗") << "\n";
        std::cout << "  - G-Buffer normals: " << (resources.gbuffer_normals.has_value() ? "✓" : "✗") << "\n";

        std::cout << "\nReady to render!\n";
        std::cout << "In a full application, this would:\n";
        std::cout << "  1. Create a window with OpenGL context\n";
        std::cout << "  2. Execute the frame graph each frame\n";
        std::cout << "  3. Present the final color output to screen\n";
        std::cout << "  4. Handle input and update the scene\n";

        std::cout << "\n✓ Geometry viewer example completed successfully\n";

        return EXIT_SUCCESS;
    }
    catch (const std::exception& e)
    {
        std::cerr << "Error: " << e.what() << "\n";
        return EXIT_FAILURE;
    }
}
