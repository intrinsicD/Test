#include <gtest/gtest.h>

#include <array>
#include <cmath>
#include <stdexcept>
#include <string_view>

#include "engine/runtime/api.hpp"

TEST(RuntimeModule, ModuleNameMatchesNamespace) {
    EXPECT_EQ(engine::runtime::module_name(), "runtime");
    EXPECT_STREQ(engine_runtime_module_name(), "runtime");
}

TEST(RuntimeModule, ExecutesSimulationPipeline) {
    engine::runtime::shutdown();
    engine::runtime::initialize();

    const auto frame = engine::runtime::tick(0.016);
    EXPECT_GE(frame.dispatch_report.execution_order.size(), 4U);
    EXPECT_EQ(frame.dispatch_report.execution_order.front(), "animation.evaluate");
    EXPECT_EQ(frame.dispatch_report.execution_order.back(), "geometry.finalize");
    ASSERT_EQ(frame.dispatch_report.execution_order.size(), frame.dispatch_report.kernel_durations.size());
    for (const auto duration : frame.dispatch_report.kernel_durations)
    {
        EXPECT_GE(duration, 0.0);
    }

    EXPECT_FALSE(frame.pose.joints.empty());
    const auto* root = frame.pose.find("root");
    ASSERT_TRUE(root != nullptr);

    const auto& mesh = engine::runtime::current_mesh();
    EXPECT_GE(mesh.bounds.max[1], mesh.bounds.min[1]);
    EXPECT_FALSE(frame.body_positions.empty());
    EXPECT_FALSE(frame.scene_nodes.empty());
    const auto& root_node = frame.scene_nodes.front();
    EXPECT_EQ(root_node.name, "root");
    const float expected_root_height = frame.body_positions.front()[1] + root->translation[1];
    EXPECT_NEAR(root_node.transform.translation[1], expected_root_height, 1e-4F);
    EXPECT_EQ(engine_runtime_scene_node_count(), frame.scene_nodes.size());
    EXPECT_STREQ(engine_runtime_scene_node_name(0), "root");
    float scales[3]{};
    float rotations[4]{};
    float translations[3]{};
    engine_runtime_scene_node_transform(0, scales, rotations, translations);
    EXPECT_FLOAT_EQ(translations[1], root_node.transform.translation[1]);

    const auto dispatch_count = engine_runtime_dispatch_count();
    ASSERT_EQ(dispatch_count, frame.dispatch_report.execution_order.size());
    for (std::size_t index = 0; index < dispatch_count; ++index)
    {
        EXPECT_DOUBLE_EQ(engine_runtime_dispatch_duration(index), frame.dispatch_report.kernel_durations[index]);
    }

    engine::runtime::shutdown();
}

TEST(RuntimeHost, EnforcesLifecycleSemantics) {
    engine::runtime::RuntimeHost host{};
    EXPECT_FALSE(host.is_initialized());
    EXPECT_THROW(host.tick(0.016), std::runtime_error);

    host.initialize();
    EXPECT_TRUE(host.is_initialized());
    const auto first_frame = host.tick(0.016);
    EXPECT_NEAR(first_frame.simulation_time, 0.016, 1e-9);

    host.shutdown();
    EXPECT_FALSE(host.is_initialized());

    host.shutdown();
    host.initialize();
    const auto second_frame = host.tick(0.008);
    EXPECT_NEAR(second_frame.simulation_time, 0.008, 1e-9);

    host.shutdown();
    EXPECT_FALSE(host.is_initialized());
}

TEST(RuntimeHost, AcceptsInjectedDependencies) {
    engine::animation::AnimationClip clip{};
    clip.name = "custom";
    clip.duration = 1.0;
    engine::animation::JointTrack track{};
    track.joint_name = "custom_joint";
    track.keyframes.push_back(engine::animation::Keyframe{});
    clip.tracks.push_back(track);

    engine::runtime::RuntimeHostDependencies deps{};
    deps.controller = engine::animation::make_linear_controller(std::move(clip));
    deps.scene_name = "custom.scene";

    engine::physics::PhysicsWorld world{};
    world.gravity = engine::math::vec3{0.0F, -1.0F, 0.0F};
    engine::physics::RigidBody body{};
    body.mass = 3.0F;
    body.position = engine::math::vec3{1.0F, 2.0F, 3.0F};
    engine::physics::add_body(world, body);
    deps.world = world;

    engine::geometry::SurfaceMesh mesh = engine::geometry::make_unit_quad();
    engine::geometry::apply_uniform_translation(mesh, engine::math::vec3{0.0F, 2.0F, 0.0F});
    engine::geometry::update_bounds(mesh);
    deps.mesh = mesh;

    engine::runtime::RuntimeHost host{deps};
    host.initialize();
    EXPECT_TRUE(host.is_initialized());
    EXPECT_FALSE(host.body_positions().empty());
    ASSERT_FALSE(host.joint_names().empty());
    EXPECT_EQ(host.joint_names().front(), "custom_joint");
    EXPECT_NEAR(host.current_mesh().bounds.min[1], mesh.bounds.min[1], 1e-5F);
    host.shutdown();
}

TEST(RuntimeModule, EnumeratesAllEngineModules) {
    constexpr std::array expected{
        std::string_view{"animation"},
        std::string_view{"assets"},
        std::string_view{"compute"},
        std::string_view{"compute.cuda"},
        std::string_view{"core"},
        std::string_view{"geometry"},
        std::string_view{"io"},
        std::string_view{"physics"},
        std::string_view{"platform"},
        std::string_view{"rendering"},
        std::string_view{"scene"},
    };

    ASSERT_EQ(engine::runtime::module_count(), expected.size());
    EXPECT_EQ(engine_runtime_module_count(), expected.size());

    for (std::size_t index = 0; index < expected.size(); ++index) {
        EXPECT_EQ(engine::runtime::module_name_at(index), expected[index]);
        EXPECT_STREQ(engine_runtime_module_at(index), expected[index].data());
    }

    EXPECT_TRUE(engine::runtime::module_name_at(expected.size()).empty());
    EXPECT_EQ(engine_runtime_module_at(expected.size()), nullptr);
}
