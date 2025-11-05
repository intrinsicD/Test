#include <gtest/gtest.h>

#include <string>

#include "engine/assets/handles.hpp"
#include "engine/rendering/command_encoder.hpp"

namespace
{
    using engine::rendering::CommandBufferHandle;
    using engine::rendering::CommandEncoderDescriptor;
    using engine::rendering::QueueType;
}

TEST(RecordingCommandEncoder, RecordsDrawsAndDispatches)
{
    engine::rendering::RecordingCommandEncoder encoder;

    engine::rendering::GeometryDrawCommand draw{};
    draw.geometry = engine::assets::MeshHandle{std::string{"mesh.handle"}};
    draw.material = engine::assets::MaterialHandle{std::string{"material.handle"}};
    encoder.draw_geometry(draw);

    engine::rendering::ComputeDispatchCommand dispatch{};
    dispatch.group_count_x = 4U;
    dispatch.group_count_y = 2U;
    dispatch.group_count_z = 1U;
    encoder.dispatch_compute(dispatch);

    const auto& commands = encoder.commands();
    ASSERT_EQ(commands.size(), 2U); // NOLINT
    EXPECT_TRUE(commands.front().is_draw());
    EXPECT_TRUE(commands.back().is_dispatch());

    const auto& draws = encoder.geometry_draws();
    ASSERT_EQ(draws.size(), 1U); // NOLINT
    EXPECT_EQ(std::get<engine::assets::MeshHandle>(draws.front().geometry).id(),
              std::string{"mesh.handle"});
    EXPECT_EQ(draws.front().material.id(), std::string{"material.handle"});

    const auto& dispatches = encoder.compute_dispatches();
    ASSERT_EQ(dispatches.size(), 1U); // NOLINT
    EXPECT_EQ(dispatches.front().group_count_x, 4U);
    EXPECT_EQ(dispatches.front().group_count_y, 2U);
    EXPECT_EQ(dispatches.front().group_count_z, 1U);

    encoder.clear();
    EXPECT_TRUE(encoder.commands().empty());
    EXPECT_TRUE(encoder.geometry_draws().empty());
    EXPECT_TRUE(encoder.compute_dispatches().empty());
}

TEST(RecordingCommandEncoderProvider, TracksDescriptorsAndEncoders)
{
    engine::rendering::RecordingCommandEncoderProvider provider;

    CommandBufferHandle handle{};
    handle.index = 7U;

    CommandEncoderDescriptor descriptor{"PassName", QueueType::Compute, handle};
    auto encoder = provider.begin_encoder(descriptor);
    ASSERT_NE(encoder, nullptr);

    encoder->dispatch_compute(engine::rendering::ComputeDispatchCommand{});

    provider.end_encoder(descriptor, std::move(encoder));

    ASSERT_EQ(provider.begin_records().size(), 1U); // NOLINT
    EXPECT_EQ(provider.begin_records().front().pass_name, "PassName");
    EXPECT_EQ(provider.begin_records().front().queue, QueueType::Compute);
    EXPECT_EQ(provider.begin_records().front().command_buffer.index, handle.index);

    ASSERT_EQ(provider.end_records().size(), 1U); // NOLINT
    EXPECT_EQ(provider.end_records().front().pass_name, "PassName");

    const auto& completed = provider.completed_encoders();
    ASSERT_EQ(completed.size(), 1U); // NOLINT
    ASSERT_EQ(completed.front()->commands().size(), 1U); // NOLINT
    EXPECT_TRUE(completed.front()->commands().front().is_dispatch());

    auto released = provider.release_completed_encoders();
    ASSERT_EQ(released.size(), 1U); // NOLINT
    EXPECT_TRUE(provider.completed_encoders().empty());

    provider.clear();
    EXPECT_TRUE(provider.begin_records().empty());
    EXPECT_TRUE(provider.end_records().empty());
}
