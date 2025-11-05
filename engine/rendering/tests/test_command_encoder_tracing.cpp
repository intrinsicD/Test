#include <gtest/gtest.h>

#include <string>
#include <vector>

#include "command_encoder_test_utils.hpp"
#include "engine/rendering/command_encoder_tracing.hpp"

namespace
{
    using engine::rendering::CommandBufferHandle;
    using engine::rendering::CommandEncoderDescriptor;
    using engine::rendering::QueueType;
}

TEST(CommandEncoderTracing, RecordsDrawAndDispatchCounts)
{
    engine::rendering::tests::RecordingCommandEncoderProvider base_provider;
    engine::rendering::TracingCommandEncoderProvider tracer(base_provider);

    CommandBufferHandle handle{};
    handle.index = 42U;
    CommandEncoderDescriptor descriptor{"TracePass", QueueType::Graphics, handle};

    auto encoder = tracer.begin_encoder(descriptor);
    ASSERT_NE(encoder, nullptr);

    engine::rendering::GeometryDrawCommand draw{};
    draw.geometry = engine::assets::MeshHandle{std::string{"trace.mesh"}};
    draw.material = engine::assets::MaterialHandle{std::string{"trace.material"}};
    encoder->draw_geometry(draw);

    engine::rendering::ComputeDispatchCommand dispatch{};
    dispatch.group_count_x = 4U;
    dispatch.group_count_y = 1U;
    dispatch.group_count_z = 2U;
    encoder->dispatch_compute(dispatch);

    tracer.end_encoder(descriptor, std::move(encoder));

    auto records = tracer.consume_records();
    ASSERT_EQ(records.size(), 1U); // NOLINT
    const auto& record = records.front();
    EXPECT_EQ(record.pass_name, "TracePass");
    EXPECT_EQ(record.queue, QueueType::Graphics);
    EXPECT_EQ(record.command_buffer.index, handle.index);
    EXPECT_EQ(record.draw_count, 1U);
    EXPECT_EQ(record.dispatch_count, 1U);

    ASSERT_EQ(base_provider.completed_encoders.size(), 1U); // NOLINT
    const auto& completed = *base_provider.completed_encoders.front();
    EXPECT_EQ(completed.commands.size(), 2U);
}

TEST(CommandEncoderTracing, ConsumeRecordsClearsState)
{
    engine::rendering::tests::RecordingCommandEncoderProvider base_provider;
    engine::rendering::TracingCommandEncoderProvider tracer(base_provider);

    CommandEncoderDescriptor descriptor{"EmptyPass", QueueType::Graphics, {}};
    auto encoder = tracer.begin_encoder(descriptor);
    tracer.end_encoder(descriptor, std::move(encoder));

    auto first = tracer.consume_records();
    ASSERT_EQ(first.size(), 1U); // NOLINT
    EXPECT_TRUE(tracer.consume_records().empty());
}

