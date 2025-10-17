#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

#include "engine/animation/api.hpp"
#include "engine/animation/rigging/rig_binding.hpp"
#include "engine/assets/mesh_asset.hpp"
#include "engine/geometry/api.hpp"
#include "engine/geometry/deform/linear_blend_skinning.hpp"
#include "engine/geometry/mesh/surface_mesh_conversion.hpp"
#include "engine/io/geometry_io.hpp"
#include "engine/math/math.hpp"
#include "engine/rendering/components.hpp"
#include "engine/rendering/frame_graph.hpp"
#include "engine/rendering/render_pass.hpp"
#include "engine/rendering/backend/vulkan/gpu_scheduler.hpp"
#include "engine/rendering/material_system.hpp"
#include "engine/rendering/resources/recording_gpu_resource_provider.hpp"
#include "command_encoder_test_utils.hpp"
#include "engine/runtime/api.hpp"

namespace
{
    class RecordingRenderResourceProvider final : public engine::rendering::RenderResourceProvider
    {
    public:
        void require_mesh(const engine::assets::MeshHandle& handle) override
        {
            meshes.push_back(handle);
        }

        void require_graph(const engine::assets::GraphHandle& handle) override
        {
            graphs.push_back(handle);
        }

        void require_point_cloud(const engine::assets::PointCloudHandle& handle) override
        {
            point_clouds.push_back(handle);
        }

        void require_material(const engine::assets::MaterialHandle& handle) override
        {
            materials.push_back(handle);
        }

        void require_shader(const engine::assets::ShaderHandle& handle) override
        {
            shaders.push_back(handle);
        }

        std::vector<engine::assets::MeshHandle> meshes{};
        std::vector<engine::assets::GraphHandle> graphs{};
        std::vector<engine::assets::PointCloudHandle> point_clouds{};
        std::vector<engine::assets::MaterialHandle> materials{};
        std::vector<engine::assets::ShaderHandle> shaders{};
    };

    class ScopedTempFile
    {
    public:
        explicit ScopedTempFile(std::string extension)
        {
            static std::atomic_uint64_t counter{0};
            const auto id = counter.fetch_add(1, std::memory_order_relaxed);
            const auto filename = "engine-integration-" + std::to_string(id) + std::move(extension);
            path_ = std::filesystem::temp_directory_path() / filename;
        }

        ScopedTempFile(const ScopedTempFile&) = delete;
        ScopedTempFile& operator=(const ScopedTempFile&) = delete;
        ScopedTempFile(ScopedTempFile&&) = delete;
        ScopedTempFile& operator=(ScopedTempFile&&) = delete;

        ~ScopedTempFile()
        {
            std::error_code ec{};
            std::filesystem::remove(path_, ec);
        }

        [[nodiscard]] const std::filesystem::path& path() const noexcept
        {
            return path_;
        }

    private:
        std::filesystem::path path_{};
    };

    [[nodiscard]] engine::animation::RigBinding make_uniform_binding(std::size_t vertex_count)
    {
        engine::animation::RigBinding binding{};
        engine::animation::RigJoint root{};
        root.name = "root";
        root.parent = engine::animation::RigBinding::kInvalidIndex;
        root.inverse_bind_pose = engine::math::Transform<float>::Identity();
        binding.joints.push_back(root);
        binding.resize_vertices(vertex_count);
        for (auto& vertex : binding.vertices)
        {
            vertex.clear();
            [[maybe_unused]] const bool added = vertex.add_influence(0U, 1.0F);
            (void)added;
            vertex.normalize_weights();
        }
        return binding;
    }
}

TEST(EngineIntegration, AnimationPhysicsRuntimePipeline)
{
    engine::runtime::RuntimeHost host{};
    host.initialize();

    const auto initial_positions = host.body_positions();
    ASSERT_FALSE(initial_positions.empty());

    engine::runtime::runtime_frame_state frame{};
    constexpr int tick_count = 8;
    constexpr double dt = 0.016;
    for (int i = 0; i < tick_count; ++i)
    {
        frame = host.tick(dt);
    }

    const std::vector<std::string> expected_order{
        "animation.evaluate",
        "physics.accumulate",
        "physics.integrate",
        "geometry.deform",
        "geometry.finalize",
    };
    ASSERT_EQ(frame.dispatch_report.execution_order.size(), expected_order.size());
    for (std::size_t index = 0; index < expected_order.size(); ++index)
    {
        EXPECT_EQ(frame.dispatch_report.execution_order[index], expected_order[index]);
    }

    ASSERT_FALSE(frame.body_positions.empty());
    ASSERT_EQ(frame.body_positions.size(), initial_positions.size());
    const auto& final_position = frame.body_positions.front();
    const float position_delta = std::abs(final_position[1] - initial_positions.front()[1]);
    EXPECT_GT(position_delta, 0.01F);

    const auto* root_pose = frame.pose.find("root");
    ASSERT_NE(root_pose, nullptr);
    EXPECT_GT(root_pose->translation[1], 0.0F);

    EXPECT_FALSE(frame.scene_nodes.empty());

    host.shutdown();
}

TEST(EngineIntegration, RuntimeConsumesMeshAssetsRoundTrip)
{
    engine::geometry::SurfaceMesh original = engine::geometry::make_unit_quad();
    const engine::math::vec3 translation{0.25F, 0.5F, -0.125F};
    for (std::size_t index = 0; index < original.rest_positions.size(); ++index)
    {
        original.rest_positions[index] += translation;
        original.positions[index] = original.rest_positions[index];
    }
    engine::geometry::recompute_vertex_normals(original);
    engine::geometry::update_bounds(original);

    ScopedTempFile temp{".obj"};
    engine::geometry::save_surface_mesh(original, temp.path());

    engine::assets::MeshCache cache;
    engine::assets::MeshAssetDescriptor descriptor = engine::assets::MeshAssetDescriptor::from_file(
        temp.path(), engine::io::MeshFileFormat::obj);
    const auto& asset = cache.load(descriptor);
    const auto loaded_surface = engine::geometry::mesh::build_surface_mesh_from_halfedge(asset.mesh.interface);

    ASSERT_EQ(loaded_surface.positions.size(), original.positions.size());
    ASSERT_EQ(loaded_surface.indices.size(), original.indices.size());
    for (std::size_t i = 0; i < original.positions.size(); ++i)
    {
        EXPECT_FLOAT_EQ(loaded_surface.positions[i][0], original.positions[i][0]);
        EXPECT_FLOAT_EQ(loaded_surface.positions[i][1], original.positions[i][1]);
        EXPECT_FLOAT_EQ(loaded_surface.positions[i][2], original.positions[i][2]);
    }
    for (std::size_t i = 0; i < original.indices.size(); ++i)
    {
        EXPECT_EQ(loaded_surface.indices[i], original.indices[i]);
    }

    engine::runtime::RuntimeHostDependencies deps{};
    deps.mesh = loaded_surface;
    deps.binding = make_uniform_binding(deps.mesh.rest_positions.size());

    engine::runtime::RuntimeHost host{std::move(deps)};
    host.initialize();

    const auto& runtime_mesh = host.current_mesh();
    ASSERT_EQ(runtime_mesh.rest_positions.size(), original.rest_positions.size());
    for (std::size_t i = 0; i < original.rest_positions.size(); ++i)
    {
        EXPECT_FLOAT_EQ(runtime_mesh.rest_positions[i][0], original.rest_positions[i][0]);
        EXPECT_FLOAT_EQ(runtime_mesh.rest_positions[i][1], original.rest_positions[i][1]);
        EXPECT_FLOAT_EQ(runtime_mesh.rest_positions[i][2], original.rest_positions[i][2]);
    }

    const auto frame = host.tick(0.016);
    EXPECT_FLOAT_EQ(frame.bounds.min[0], runtime_mesh.bounds.min[0]);
    EXPECT_FLOAT_EQ(frame.bounds.max[0], runtime_mesh.bounds.max[0]);

    host.shutdown();
}

TEST(EngineIntegration, RuntimeSubmitsFrameGraphThroughVulkanScheduler)
{
    engine::runtime::RuntimeHostDependencies deps{};
    deps.binding = make_uniform_binding(deps.mesh.rest_positions.size());
    deps.render_geometry = engine::rendering::components::RenderGeometry::from_mesh(
        engine::assets::MeshHandle{std::string{"integration.runtime.mesh"}},
        engine::assets::MaterialHandle{std::string{"integration.runtime.material"}});
    deps.renderable_name = "integration.runtime.renderable";

    engine::runtime::RuntimeHost host{std::move(deps)};
    host.initialize();
    const auto frame = host.tick(0.016);
    ASSERT_FALSE(frame.scene_nodes.empty());

    engine::rendering::MaterialSystem materials;
    materials.register_material(engine::rendering::MaterialSystem::MaterialRecord{
        engine::assets::MaterialHandle{std::string{"integration.runtime.material"}},
        engine::assets::ShaderHandle{std::string{"integration.runtime.shader"}},
    });

    RecordingRenderResourceProvider resources;
    engine::rendering::resources::RecordingGpuResourceProvider device(
        engine::rendering::resources::GraphicsApi::Vulkan);
    engine::rendering::backend::vulkan::VulkanGpuScheduler scheduler(device);
    engine::rendering::tests::RecordingCommandEncoderProvider encoders;
    engine::rendering::FrameGraph graph;

    engine::runtime::RuntimeHost::RenderSubmissionContext context{
        resources,
        materials,
        device,
        scheduler,
        encoders,
        graph,
        nullptr,
    };

    host.submit_render_graph(context);

    const auto& submissions = scheduler.submissions();
    ASSERT_EQ(submissions.size(), 1U);
    const auto& submission = submissions.front();
    EXPECT_EQ(submission.pass_name, "ForwardGeometry");
    EXPECT_EQ(submission.command_buffer.queue.api, engine::rendering::resources::GraphicsApi::Vulkan);

    ASSERT_EQ(encoders.completed_encoders.size(), 1U);
    const auto& encoder = *encoders.completed_encoders.front();
    ASSERT_EQ(encoder.draws.size(), 1U);
    const auto& draw = encoder.draws.front();
    ASSERT_TRUE(std::holds_alternative<engine::assets::MeshHandle>(draw.geometry));
    const auto& mesh_handle = std::get<engine::assets::MeshHandle>(draw.geometry);
    EXPECT_EQ(mesh_handle.id(), std::string{"integration.runtime.mesh"});
    EXPECT_EQ(draw.material.id(), std::string{"integration.runtime.material"});

    ASSERT_EQ(resources.meshes.size(), 1U);
    EXPECT_EQ(resources.meshes.front().id(), std::string{"integration.runtime.mesh"});
    ASSERT_EQ(resources.materials.size(), 1U);
    EXPECT_EQ(resources.materials.front().id(), std::string{"integration.runtime.material"});

    host.shutdown();
}
