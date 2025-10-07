#include <gtest/gtest.h>

#include <string>
#include <vector>

#include "engine/rendering/components.hpp"
#include "engine/rendering/forward_pipeline.hpp"
#include "engine/scene/components/transform.hpp"
#include "engine/scene/scene.hpp"

namespace
{
    class RecordingProvider final : public engine::rendering::RenderResourceProvider
    {
    public:
        void require_mesh(const engine::assets::MeshHandle& handle) override
        {
            meshes.push_back(handle);
        }

        void require_material(const engine::assets::MaterialHandle& handle) override
        {
            materials.push_back(handle);
        }

        void require_shader(const engine::assets::ShaderHandle& handle) override
        {
            shaders.push_back(handle);
        }

        std::vector<engine::assets::MeshHandle> meshes;
        std::vector<engine::assets::MaterialHandle> materials;
        std::vector<engine::assets::ShaderHandle> shaders;
    };
}

TEST(ForwardPipeline, RequestsResourcesForVisibleRenderables)
{
    engine::scene::Scene scene;
    const auto entity = scene.create_entity();

    auto& world = scene.registry().emplace<engine::scene::components::WorldTransform>(entity.id());
    world.value.translation = engine::math::Vector<float, 3>{1.0F, 2.0F, 3.0F};

    auto& geometry = scene.registry().emplace<engine::rendering::components::RenderGeometry>(entity.id());
    geometry.mesh = engine::assets::MeshHandle{std::string{"mesh"}};
    geometry.material = engine::assets::MaterialHandle{std::string{"material"}};

    engine::rendering::MaterialSystem materials;
    materials.register_material(engine::rendering::MaterialSystem::MaterialRecord{
        engine::assets::MaterialHandle{std::string{"material"}},
        engine::assets::ShaderHandle{std::string{"shader"}}
    });

    engine::rendering::FrameGraph graph;
    engine::rendering::ForwardPipeline pipeline;
    RecordingProvider provider;

    pipeline.render(scene, provider, materials, graph);

    ASSERT_EQ(graph.execution_order().size(), 1);  // NOLINT
    const auto& events = graph.resource_events();
    ASSERT_EQ(events.size(), 4);  // NOLINT

    EXPECT_EQ(events[0].resource_name, "ForwardColor");
    EXPECT_EQ(events[0].type, engine::rendering::ResourceEvent::Type::Acquire);

    EXPECT_EQ(events[1].resource_name, "ForwardDepth");
    EXPECT_EQ(events[1].type, engine::rendering::ResourceEvent::Type::Acquire);

    EXPECT_EQ(events[2].resource_name, "ForwardColor");
    EXPECT_EQ(events[2].type, engine::rendering::ResourceEvent::Type::Release);

    EXPECT_EQ(events[3].resource_name, "ForwardDepth");
    EXPECT_EQ(events[3].type, engine::rendering::ResourceEvent::Type::Release);

    ASSERT_EQ(provider.meshes.size(), 1);  // NOLINT
    EXPECT_EQ(provider.meshes.front().id(), std::string{"mesh"});

    ASSERT_EQ(provider.materials.size(), 1);  // NOLINT
    EXPECT_EQ(provider.materials.front().id(), std::string{"material"});

    ASSERT_EQ(provider.shaders.size(), 1);  // NOLINT
    EXPECT_EQ(provider.shaders.front().id(), std::string{"shader"});
}
