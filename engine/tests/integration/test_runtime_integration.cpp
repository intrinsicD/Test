#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>
#include <type_traits>

#include "engine/animation/api.hpp"
#include "engine/animation/rigging/rig_binding.hpp"
#include "engine/assets/mesh_asset.hpp"
#include "engine/assets/validation.hpp"
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

    class ScopedHandleValidators
    {
    public:
        ScopedHandleValidators()
        {
            auto& registry = engine::assets::HandleValidatorRegistry::instance();
            mesh_ = registry.register_mesh_validator(
                [](const engine::assets::MeshHandle&) { return true; });
            graph_ = registry.register_graph_validator(
                [](const engine::assets::GraphHandle&) { return true; });
            point_cloud_ = registry.register_point_cloud_validator(
                [](const engine::assets::PointCloudHandle&) { return true; });
            material_ = registry.register_material_validator(
                [](const engine::assets::MaterialHandle&) { return true; });
            shader_ = registry.register_shader_validator(
                [](const engine::assets::ShaderHandle&) { return true; });
        }

    private:
        std::shared_ptr<void> mesh_{};
        std::shared_ptr<void> graph_{};
        std::shared_ptr<void> point_cloud_{};
        std::shared_ptr<void> material_{};
        std::shared_ptr<void> shader_{};
    };

    template <typename Handle>
    Handle make_bound_handle(std::string identifier)
    {
        static std::atomic_uint32_t next_index{0};
        Handle handle{std::move(identifier)};
        typename Handle::pool_handle_type raw{};
        raw.index = next_index.fetch_add(1U, std::memory_order_relaxed);
        if (raw.index == Handle::pool_handle_type::invalid_index)
        {
            raw.index = 0U;
        }
        raw.generation = 1U;
        handle.bind(raw);
        return handle;
    }

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

    struct VulkanSubmissionSnapshot
    {
        std::string pass_name{};
        engine::rendering::resources::QueueNativeHandle queue{};
        engine::rendering::resources::CommandBufferNativeHandle command_buffer{};
        std::vector<engine::rendering::resources::Barrier> begin_barriers{};
        std::vector<engine::rendering::resources::Barrier> end_barriers{};
    };

    struct DrawSnapshot
    {
        std::string geometry_id{};
        std::string material_id{};
        engine::math::Transform<float> transform{};
    };

    struct ResourceInfoSnapshot
    {
        std::size_t handle_index{0};
        std::string name{};
        engine::rendering::ResourceFormat format{engine::rendering::ResourceFormat::Unknown};
        engine::rendering::ResourceDimension dimension{engine::rendering::ResourceDimension::Unknown};
        engine::rendering::ResourceUsage usage{engine::rendering::ResourceUsage::None};
        engine::rendering::ResourceState initial_state{engine::rendering::ResourceState::Undefined};
        engine::rendering::ResourceState final_state{engine::rendering::ResourceState::Undefined};
        std::uint32_t width{0};
        std::uint32_t height{0};
        std::uint32_t depth{0};
        std::uint32_t array_layers{0};
        std::uint32_t mip_levels{0};
        engine::rendering::ResourceSampleCount sample_count{engine::rendering::ResourceSampleCount::Count1};
        std::uint64_t size_bytes{0};
    };

    struct SubmissionSnapshot
    {
        std::vector<VulkanSubmissionSnapshot> submissions{};
        std::vector<DrawSnapshot> draws{};
        std::vector<std::string> required_meshes{};
        std::vector<std::string> required_materials{};
        std::vector<ResourceInfoSnapshot> acquired_resources{};
        std::vector<ResourceInfoSnapshot> released_resources{};
        std::vector<engine::rendering::tests::RecordingCommandEncoderProvider::DescriptorRecord> begin_records{};
        std::vector<engine::rendering::tests::RecordingCommandEncoderProvider::DescriptorRecord> end_records{};
        std::string frame_graph_serialization{};
        std::size_t frames_begun{0};
        std::size_t frames_completed{0};
    };

    SubmissionSnapshot capture_submission_snapshot(engine::runtime::RuntimeHost& host,
                                                   engine::rendering::MaterialSystem& materials)
    {
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

        SubmissionSnapshot snapshot{};
        snapshot.frame_graph_serialization = graph.serialize();
        snapshot.frames_begun = device.frames_begun();
        snapshot.frames_completed = device.frames_completed();
        snapshot.begin_records = encoders.begin_records;
        snapshot.end_records = encoders.end_records;

        const auto& submissions = scheduler.submissions();
        snapshot.submissions.reserve(submissions.size());
        for (const auto& submission : submissions)
        {
            VulkanSubmissionSnapshot record{};
            record.pass_name = submission.pass_name;
            record.queue = submission.command_buffer.queue;
            record.command_buffer = submission.command_buffer.command_buffer;
            record.begin_barriers = submission.begin_barriers;
            record.end_barriers = submission.end_barriers;
            snapshot.submissions.push_back(std::move(record));
        }

        for (const auto& encoder_ptr : encoders.completed_encoders)
        {
            const auto& encoder = *encoder_ptr;
            snapshot.draws.reserve(snapshot.draws.size() + encoder.draws.size());
            for (const auto& draw : encoder.draws)
            {
                DrawSnapshot draw_snapshot{};
                draw_snapshot.geometry_id = std::visit(
                    [](const auto& value) -> std::string {
                        using ValueType = std::decay_t<decltype(value)>;
                        if constexpr (std::is_same_v<ValueType, std::monostate>)
                        {
                            return std::string{"<empty>"};
                        }
                        else if constexpr (std::is_same_v<ValueType, engine::assets::MeshHandle> ||
                                           std::is_same_v<ValueType, engine::assets::GraphHandle> ||
                                           std::is_same_v<ValueType, engine::assets::PointCloudHandle>)
                        {
                            return value.id();
                        }
                        else
                        {
                            return std::string{"<unknown>"};
                        }
                    },
                    draw.geometry);
                draw_snapshot.material_id = draw.material.id();
                draw_snapshot.transform = draw.transform;
                snapshot.draws.push_back(std::move(draw_snapshot));
            }
        }

        snapshot.required_meshes.reserve(resources.meshes.size());
        for (const auto& handle : resources.meshes)
        {
            snapshot.required_meshes.push_back(handle.id());
        }
        snapshot.required_materials.reserve(resources.materials.size());
        for (const auto& handle : resources.materials)
        {
            snapshot.required_materials.push_back(handle.id());
        }

        const auto capture_resource_records = [](const auto& records) {
            std::vector<ResourceInfoSnapshot> snapshot_records{};
            snapshot_records.reserve(records.size());
            for (const auto& record : records)
            {
                ResourceInfoSnapshot info{};
                info.handle_index = record.handle.index;
                info.name = std::string{record.info.name};
                info.format = record.info.format;
                info.dimension = record.info.dimension;
                info.usage = record.info.usage;
                info.initial_state = record.info.initial_state;
                info.final_state = record.info.final_state;
                info.width = record.info.width;
                info.height = record.info.height;
                info.depth = record.info.depth;
                info.array_layers = record.info.array_layers;
                info.mip_levels = record.info.mip_levels;
                info.sample_count = record.info.sample_count;
                info.size_bytes = record.info.size_bytes;
                snapshot_records.push_back(std::move(info));
            }
            return snapshot_records;
        };

        snapshot.acquired_resources = capture_resource_records(device.acquired());
        snapshot.released_resources = capture_resource_records(device.released());

        return snapshot;
    }

    void expect_equal_barrier(const engine::rendering::resources::Barrier& expected,
                              const engine::rendering::resources::Barrier& actual)
    {
        EXPECT_EQ(actual.resource, expected.resource);
        EXPECT_EQ(actual.source_stage, expected.source_stage);
        EXPECT_EQ(actual.destination_stage, expected.destination_stage);
        EXPECT_EQ(actual.source_access, expected.source_access);
        EXPECT_EQ(actual.destination_access, expected.destination_access);
    }

    void expect_equal_transform(const engine::math::Transform<float>& expected,
                                const engine::math::Transform<float>& actual)
    {
        for (int axis = 0; axis < 3; ++axis)
        {
            EXPECT_FLOAT_EQ(actual.scale[axis], expected.scale[axis]);
            EXPECT_FLOAT_EQ(actual.translation[axis], expected.translation[axis]);
        }
        EXPECT_FLOAT_EQ(actual.rotation.w, expected.rotation.w);
        EXPECT_FLOAT_EQ(actual.rotation.x, expected.rotation.x);
        EXPECT_FLOAT_EQ(actual.rotation.y, expected.rotation.y);
        EXPECT_FLOAT_EQ(actual.rotation.z, expected.rotation.z);
    }

    void expect_equal_resource_info(const ResourceInfoSnapshot& expected,
                                    const ResourceInfoSnapshot& actual)
    {
        EXPECT_EQ(actual.handle_index, expected.handle_index);
        EXPECT_EQ(actual.name, expected.name);
        EXPECT_EQ(actual.format, expected.format);
        EXPECT_EQ(actual.dimension, expected.dimension);
        EXPECT_EQ(actual.usage, expected.usage);
        EXPECT_EQ(actual.initial_state, expected.initial_state);
        EXPECT_EQ(actual.final_state, expected.final_state);
        EXPECT_EQ(actual.width, expected.width);
        EXPECT_EQ(actual.height, expected.height);
        EXPECT_EQ(actual.depth, expected.depth);
        EXPECT_EQ(actual.array_layers, expected.array_layers);
        EXPECT_EQ(actual.mip_levels, expected.mip_levels);
        EXPECT_EQ(actual.sample_count, expected.sample_count);
        EXPECT_EQ(actual.size_bytes, expected.size_bytes);
    }

    void expect_equal_submission_snapshot(const SubmissionSnapshot& expected,
                                          const SubmissionSnapshot& actual)
    {
        EXPECT_EQ(actual.frame_graph_serialization, expected.frame_graph_serialization);
        EXPECT_EQ(actual.frames_begun, expected.frames_begun);
        EXPECT_EQ(actual.frames_completed, expected.frames_completed);

        ASSERT_EQ(actual.begin_records.size(), expected.begin_records.size());
        for (std::size_t index = 0; index < expected.begin_records.size(); ++index)
        {
            EXPECT_EQ(actual.begin_records[index].pass_name, expected.begin_records[index].pass_name);
            EXPECT_EQ(actual.begin_records[index].queue, expected.begin_records[index].queue);
            EXPECT_EQ(actual.begin_records[index].command_buffer, expected.begin_records[index].command_buffer);
        }

        ASSERT_EQ(actual.end_records.size(), expected.end_records.size());
        for (std::size_t index = 0; index < expected.end_records.size(); ++index)
        {
            EXPECT_EQ(actual.end_records[index].pass_name, expected.end_records[index].pass_name);
            EXPECT_EQ(actual.end_records[index].queue, expected.end_records[index].queue);
            EXPECT_EQ(actual.end_records[index].command_buffer, expected.end_records[index].command_buffer);
        }

        ASSERT_EQ(actual.submissions.size(), expected.submissions.size());
        for (std::size_t submission_index = 0; submission_index < expected.submissions.size(); ++submission_index)
        {
            const auto& actual_submission = actual.submissions[submission_index];
            const auto& expected_submission = expected.submissions[submission_index];
            EXPECT_EQ(actual_submission.pass_name, expected_submission.pass_name);
            EXPECT_EQ(actual_submission.queue.api, expected_submission.queue.api);
            EXPECT_EQ(actual_submission.queue.value, expected_submission.queue.value);
            EXPECT_EQ(actual_submission.queue.queue, expected_submission.queue.queue);
            EXPECT_EQ(actual_submission.command_buffer.api, expected_submission.command_buffer.api);
            EXPECT_EQ(actual_submission.command_buffer.value, expected_submission.command_buffer.value);
            EXPECT_EQ(actual_submission.command_buffer.queue, expected_submission.command_buffer.queue);
            EXPECT_EQ(actual_submission.command_buffer.label, expected_submission.command_buffer.label);
            EXPECT_EQ(actual_submission.command_buffer.index, expected_submission.command_buffer.index);

            ASSERT_EQ(actual_submission.begin_barriers.size(), expected_submission.begin_barriers.size());
            for (std::size_t barrier_index = 0; barrier_index < expected_submission.begin_barriers.size(); ++barrier_index)
            {
                expect_equal_barrier(expected_submission.begin_barriers[barrier_index],
                                     actual_submission.begin_barriers[barrier_index]);
            }

            ASSERT_EQ(actual_submission.end_barriers.size(), expected_submission.end_barriers.size());
            for (std::size_t barrier_index = 0; barrier_index < expected_submission.end_barriers.size(); ++barrier_index)
            {
                expect_equal_barrier(expected_submission.end_barriers[barrier_index],
                                     actual_submission.end_barriers[barrier_index]);
            }
        }

        ASSERT_EQ(actual.draws.size(), expected.draws.size());
        for (std::size_t draw_index = 0; draw_index < expected.draws.size(); ++draw_index)
        {
            const auto& actual_draw = actual.draws[draw_index];
            const auto& expected_draw = expected.draws[draw_index];
            EXPECT_EQ(actual_draw.geometry_id, expected_draw.geometry_id);
            EXPECT_EQ(actual_draw.material_id, expected_draw.material_id);
            expect_equal_transform(expected_draw.transform, actual_draw.transform);
        }

        ASSERT_EQ(actual.required_meshes.size(), expected.required_meshes.size());
        for (std::size_t index = 0; index < expected.required_meshes.size(); ++index)
        {
            EXPECT_EQ(actual.required_meshes[index], expected.required_meshes[index]);
        }

        ASSERT_EQ(actual.required_materials.size(), expected.required_materials.size());
        for (std::size_t index = 0; index < expected.required_materials.size(); ++index)
        {
            EXPECT_EQ(actual.required_materials[index], expected.required_materials[index]);
        }

        ASSERT_EQ(actual.acquired_resources.size(), expected.acquired_resources.size());
        for (std::size_t index = 0; index < expected.acquired_resources.size(); ++index)
        {
            expect_equal_resource_info(expected.acquired_resources[index], actual.acquired_resources[index]);
        }

        ASSERT_EQ(actual.released_resources.size(), expected.released_resources.size());
        for (std::size_t index = 0; index < expected.released_resources.size(); ++index)
        {
            expect_equal_resource_info(expected.released_resources[index], actual.released_resources[index]);
        }
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
    ScopedHandleValidators handle_validators{};
    engine::runtime::RuntimeHostDependencies deps{};
    deps.binding = make_uniform_binding(deps.mesh.rest_positions.size());
    deps.render_geometry = engine::rendering::components::RenderGeometry::from_mesh(
        make_bound_handle<engine::assets::MeshHandle>("integration.runtime.mesh"),
        make_bound_handle<engine::assets::MaterialHandle>("integration.runtime.material"));
    deps.renderable_name = "integration.runtime.renderable";

    engine::runtime::RuntimeHost host{std::move(deps)};
    host.initialize();
    const auto frame = host.tick(0.016);
    ASSERT_FALSE(frame.scene_nodes.empty());

    engine::rendering::MaterialSystem materials;
    materials.register_material(engine::rendering::MaterialSystem::MaterialRecord{
        make_bound_handle<engine::assets::MaterialHandle>("integration.runtime.material"),
        make_bound_handle<engine::assets::ShaderHandle>("integration.runtime.shader")});

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

TEST(EngineIntegration, RuntimeSubmissionRemainsDeterministicAcrossInvocations)
{
    ScopedHandleValidators handle_validators{};
    engine::runtime::RuntimeHostDependencies deps{};
    deps.binding = make_uniform_binding(deps.mesh.rest_positions.size());
    deps.render_geometry = engine::rendering::components::RenderGeometry::from_mesh(
        make_bound_handle<engine::assets::MeshHandle>("integration.runtime.deterministic.mesh"),
        make_bound_handle<engine::assets::MaterialHandle>("integration.runtime.deterministic.material"));
    deps.renderable_name = "integration.runtime.deterministic";

    engine::runtime::RuntimeHost host{std::move(deps)};
    host.initialize();
    host.tick(0.016);

    engine::rendering::MaterialSystem materials;
    materials.register_material(engine::rendering::MaterialSystem::MaterialRecord{
        make_bound_handle<engine::assets::MaterialHandle>("integration.runtime.deterministic.material"),
        make_bound_handle<engine::assets::ShaderHandle>("integration.runtime.deterministic.shader")});

    constexpr int iteration_count = 3;
    std::vector<SubmissionSnapshot> snapshots{};
    snapshots.reserve(iteration_count);
    for (int iteration = 0; iteration < iteration_count; ++iteration)
    {
        snapshots.push_back(capture_submission_snapshot(host, materials));
    }

    ASSERT_FALSE(snapshots.empty());
    const auto& baseline = snapshots.front();
    for (std::size_t index = 1; index < snapshots.size(); ++index)
    {
        SCOPED_TRACE(::testing::Message() << "Snapshot index=" << index);
        expect_equal_submission_snapshot(baseline, snapshots[index]);
    }

    host.shutdown();
}
