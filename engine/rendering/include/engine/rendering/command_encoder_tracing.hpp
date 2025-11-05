#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "engine/rendering/command_encoder.hpp"

namespace engine::rendering
{
    struct EncoderTraceRecord
    {
        std::string pass_name{};
        QueueType queue{QueueType::Graphics};
        CommandBufferHandle command_buffer{};
        std::uint64_t draw_count{0};
        std::uint64_t dispatch_count{0};
    };

    class TracingCommandEncoder final : public CommandEncoder
    {
    public:
        explicit TracingCommandEncoder(std::unique_ptr<CommandEncoder> inner,
                                       EncoderTraceRecord& record) noexcept;

        void draw_geometry(const GeometryDrawCommand& command) override;
        void dispatch_compute(const ComputeDispatchCommand& command) override;

        [[nodiscard]] std::unique_ptr<CommandEncoder> release() noexcept;

    private:
        std::unique_ptr<CommandEncoder> inner_{};
        EncoderTraceRecord* record_{nullptr};
    };

    class TracingCommandEncoderProvider final : public CommandEncoderProvider
    {
    public:
        explicit TracingCommandEncoderProvider(CommandEncoderProvider& inner) noexcept;

        [[nodiscard]] std::unique_ptr<CommandEncoder> begin_encoder(
            const CommandEncoderDescriptor& descriptor) override;

        void end_encoder(const CommandEncoderDescriptor& descriptor,
                         std::unique_ptr<CommandEncoder> encoder) override;

        [[nodiscard]] std::vector<EncoderTraceRecord> consume_records() noexcept;

    private:
        CommandEncoderProvider* inner_{nullptr};
        std::vector<EncoderTraceRecord> records_{};
    };
} // namespace engine::rendering

