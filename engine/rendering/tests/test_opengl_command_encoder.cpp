#include <gtest/gtest.h>

#include <optional>
#include <string>

#include "engine/assets/handles.hpp"
#include "engine/math/transform.hpp"
#include "engine/rendering/components.hpp"
#include "engine/geometry/api.hpp"
#include "engine/rendering/backend/opengl/command_encoder.hpp"
#include "engine/rendering/backend/opengl/gpu_scheduler.hpp"
#include "engine/rendering/backend/opengl/immediate_command_stream.hpp"
#include "engine/rendering/backend/opengl/render_resource_provider.hpp"
#include "engine/rendering/backend/opengl/resource_provider.hpp"

namespace
{
    class RecordingStream final : public engine::rendering::backend::opengl::CommandStream
    {
    public:
        void begin_submission(const engine::rendering::backend::opengl::OpenGLSubmission& submission) override
        {
            ++begin_count;
            last_pass = submission.pass_name;
            last_command_count = submission.commands.size();
            last_compute_dispatches = 0U;
            for (const auto& command : submission.commands)
            {
                if (command.is_dispatch())
                {
                    ++last_compute_dispatches;
                }
            }
        }

        void wait_timeline(const engine::rendering::backend::opengl::OpenGLTimelineSubmit&) override
        {
        }

        void issue_memory_barrier(std::uint32_t) override
        {
        }

        void execute_command_buffer(const engine::rendering::backend::opengl::OpenGLCommandEncoderSubmit&) override
        {
            ++execute_count;
        }

        void signal_timeline(const engine::rendering::backend::opengl::OpenGLTimelineSubmit&) override
        {
        }

        void signal_fence(engine::rendering::resources::FenceNativeHandle, std::uint64_t) override
        {
        }

        void end_submission(const engine::rendering::backend::opengl::OpenGLSubmission&) override
        {
            ++end_count;
        }

        std::size_t begin_count{0};
        std::size_t execute_count{0};
        std::size_t end_count{0};
        std::string last_pass;
        std::size_t last_command_count{0};
        std::size_t last_compute_dispatches{0};
    };
}

TEST(OpenGLCommandEncoder, RecordsDrawCommands)
{
    using namespace engine::rendering;
    using namespace engine::rendering::backend::opengl;

    OpenGLGpuResourceProvider provider;
    RecordingStream stream;
    OpenGLGpuScheduler scheduler(provider, &stream);
    OpenGLCommandEncoderProvider encoders(provider);

    auto command_buffer = scheduler.request_command_buffer(QueueType::Graphics, "EncodePass");

    CommandEncoderDescriptor descriptor{"EncodePass", QueueType::Graphics, command_buffer};
    auto encoder = encoders.begin_encoder(descriptor);

    GeometryDrawCommand draw{};
    const auto mesh = engine::assets::MeshHandle{std::string{"mesh.handle"}};
    draw.geometry = mesh;
    draw.material = engine::assets::MaterialHandle{std::string{"material.handle"}};
    draw.transform = engine::math::Transform<float>::Identity();
    encoder->draw_geometry(draw);

    encoders.end_encoder(descriptor, std::move(encoder));

    GpuSubmitInfo submit{};
    submit.pass_name = "EncodePass";
    submit.queue = QueueType::Graphics;
    submit.command_buffer = command_buffer;

    scheduler.submit(submit);
    scheduler.recycle(command_buffer);

    ASSERT_EQ(scheduler.submissions().size(), 1U); // NOLINT
    const auto& submission = scheduler.submissions().front();
    ASSERT_EQ(submission.commands.size(), 1U); // NOLINT
    const auto& recorded = submission.commands.front();
    ASSERT_EQ(recorded.type(), engine::rendering::EncodedCommand::Type::DrawGeometry);
    EXPECT_EQ(recorded.geometry_draw().material.id(), std::string{"material.handle"});
    EXPECT_EQ(recorded.geometry_draw().geometry.index(), draw.geometry.index());

    EXPECT_EQ(stream.begin_count, 1U);
    EXPECT_EQ(stream.execute_count, 1U);
    EXPECT_EQ(stream.end_count, 1U);
    EXPECT_EQ(stream.last_pass, "EncodePass");
    EXPECT_EQ(stream.last_command_count, 1U);
    EXPECT_EQ(stream.last_compute_dispatches, 0U);
}

TEST(OpenGLCommandEncoder, RecordsComputeDispatches)
{
    using namespace engine::rendering;
    using namespace engine::rendering::backend::opengl;

    OpenGLGpuResourceProvider provider;
    RecordingStream stream;
    OpenGLGpuScheduler scheduler(provider, &stream);
    OpenGLCommandEncoderProvider encoders(provider);

    auto command_buffer = scheduler.request_command_buffer(QueueType::Compute, "DispatchPass");

    CommandEncoderDescriptor descriptor{"DispatchPass", QueueType::Compute, command_buffer};
    auto encoder = encoders.begin_encoder(descriptor);

    ComputeDispatchCommand dispatch{};
    dispatch.group_count_x = 4U;
    dispatch.group_count_y = 2U;
    dispatch.group_count_z = 1U;
    encoder->dispatch_compute(dispatch);

    encoders.end_encoder(descriptor, std::move(encoder));

    GpuSubmitInfo submit{};
    submit.pass_name = "DispatchPass";
    submit.queue = QueueType::Compute;
    submit.command_buffer = command_buffer;

    scheduler.submit(submit);
    scheduler.recycle(command_buffer);

    ASSERT_EQ(scheduler.submissions().size(), 1U); // NOLINT
    const auto& submission = scheduler.submissions().front();
    ASSERT_EQ(submission.commands.size(), 1U); // NOLINT
    const auto& recorded = submission.commands.front();
    ASSERT_EQ(recorded.type(), engine::rendering::EncodedCommand::Type::DispatchCompute);
    EXPECT_EQ(recorded.compute_dispatch().group_count_x, 4U);
    EXPECT_EQ(recorded.compute_dispatch().group_count_y, 2U);
    EXPECT_EQ(recorded.compute_dispatch().group_count_z, 1U);

    EXPECT_EQ(stream.begin_count, 1U);
    EXPECT_EQ(stream.execute_count, 1U);
    EXPECT_EQ(stream.end_count, 1U);
    EXPECT_EQ(stream.last_pass, "DispatchPass");
    EXPECT_EQ(stream.last_command_count, 1U);
    EXPECT_EQ(stream.last_compute_dispatches, 1U);
}

TEST(OpenGLImmediateCommandStream, ExecutesMeshDraws)
{
    using namespace engine::rendering;
    using namespace engine::rendering::backend::opengl;

    engine::geometry::SurfaceMesh mesh{};
    mesh.positions = {
        {0.0F, 0.0F, 0.0F},
        {1.0F, 0.0F, 0.0F},
        {0.0F, 1.0F, 0.0F},
    };
    mesh.indices = {0, 1, 2};

    auto resolver = [captured_mesh = mesh](const engine::assets::MeshHandle&) -> std::optional<engine::geometry::SurfaceMesh>
    {
        return captured_mesh;
    };

    OpenGLRenderResourceProvider render_resources(resolver);
    const auto mesh_handle = engine::assets::MeshHandle{std::string{"mesh.example"}};

    OpenGLImmediateCommandStream stream(render_resources);
    OpenGLGpuResourceProvider provider;
    OpenGLGpuScheduler scheduler(provider, &stream);
    OpenGLCommandEncoderProvider encoders(provider);

    auto command_buffer = scheduler.request_command_buffer(QueueType::Graphics, "ImmediatePass");
    CommandEncoderDescriptor descriptor{"ImmediatePass", QueueType::Graphics, command_buffer};
    auto encoder = encoders.begin_encoder(descriptor);

    GeometryDrawCommand draw{};
    draw.geometry = mesh_handle;
    draw.transform = engine::math::Transform<float>::Identity();
    encoder->draw_geometry(draw);

    encoders.end_encoder(descriptor, std::move(encoder));

    GpuSubmitInfo submit{};
    submit.pass_name = "ImmediatePass";
    submit.queue = QueueType::Graphics;
    submit.command_buffer = command_buffer;

    scheduler.submit(submit);
    scheduler.recycle(command_buffer);

    EXPECT_EQ(stream.draw_call_count(), 1U);
    EXPECT_EQ(stream.compute_dispatch_count(), 0U);

    auto second_command_buffer = scheduler.request_command_buffer(QueueType::Graphics, "ImmediatePassSecond");
    CommandEncoderDescriptor second_descriptor{"ImmediatePassSecond", QueueType::Graphics, second_command_buffer};
    auto second_encoder = encoders.begin_encoder(second_descriptor);
    second_encoder->draw_geometry(draw);
    encoders.end_encoder(second_descriptor, std::move(second_encoder));

    GpuSubmitInfo second_submit{};
    second_submit.pass_name = "ImmediatePassSecond";
    second_submit.queue = QueueType::Graphics;
    second_submit.command_buffer = second_command_buffer;

    scheduler.submit(second_submit);
    scheduler.recycle(second_command_buffer);

    EXPECT_EQ(stream.draw_call_count(), 1U);
    EXPECT_EQ(stream.compute_dispatch_count(), 0U);
}

TEST(OpenGLImmediateCommandStream, ExecutesComputeDispatches)
{
    using namespace engine::rendering;
    using namespace engine::rendering::backend::opengl;

    OpenGLRenderResourceProvider render_resources([](const engine::assets::MeshHandle&) -> std::optional<engine::geometry::SurfaceMesh>
                                                 {
                                                     return std::nullopt;
                                                 });

    OpenGLImmediateCommandStream stream(render_resources);
    OpenGLGpuResourceProvider provider;
    OpenGLGpuScheduler scheduler(provider, &stream);
    OpenGLCommandEncoderProvider encoders(provider);

    auto command_buffer = scheduler.request_command_buffer(QueueType::Compute, "ComputePass");
    CommandEncoderDescriptor descriptor{"ComputePass", QueueType::Compute, command_buffer};
    auto encoder = encoders.begin_encoder(descriptor);

    ComputeDispatchCommand dispatch{};
    dispatch.group_count_x = 8U;
    dispatch.group_count_y = 1U;
    dispatch.group_count_z = 2U;
    encoder->dispatch_compute(dispatch);

    encoders.end_encoder(descriptor, std::move(encoder));

    GpuSubmitInfo submit{};
    submit.pass_name = "ComputePass";
    submit.queue = QueueType::Compute;
    submit.command_buffer = command_buffer;

    scheduler.submit(submit);
    scheduler.recycle(command_buffer);

    EXPECT_EQ(stream.compute_dispatch_count(), 1U);
}
