#pragma once

#include <memory>
#include <string>
#include <vector>

#include "engine/rendering/command_encoder.hpp"

namespace engine::rendering::tests
{
    /// Command encoder that records geometry draws for assertions.
    class RecordingCommandEncoder final : public CommandEncoder
    {
    public:
        void draw_geometry(const GeometryDrawCommand& command) override
        {
            draws.push_back(command);
        }

        std::vector<GeometryDrawCommand> draws;
    };

    /// Provider that captures begin/end calls and keeps completed encoders alive.
    class RecordingCommandEncoderProvider final : public CommandEncoderProvider
    {
    public:
        struct DescriptorRecord
        {
            std::string pass_name;
            QueueType queue{QueueType::Graphics};
            CommandBufferHandle command_buffer{};
        };

        std::unique_ptr<CommandEncoder> begin_encoder(const CommandEncoderDescriptor& descriptor) override
        {
            begin_records.push_back(DescriptorRecord{std::string{descriptor.pass_name}, descriptor.queue,
                                                     descriptor.command_buffer});
            return std::make_unique<RecordingCommandEncoder>();
        }

        void end_encoder(const CommandEncoderDescriptor& descriptor,
                          std::unique_ptr<CommandEncoder> encoder) override
        {
            end_records.push_back(DescriptorRecord{std::string{descriptor.pass_name}, descriptor.queue,
                                                   descriptor.command_buffer});
            auto* recording = dynamic_cast<RecordingCommandEncoder*>(encoder.release());
            if (recording != nullptr)
            {
                completed_encoders.emplace_back(recording);
            }
        }

        std::vector<DescriptorRecord> begin_records;
        std::vector<DescriptorRecord> end_records;
        std::vector<std::unique_ptr<RecordingCommandEncoder>> completed_encoders;
    };

    /// Encoder that ignores all draw calls.
    class NullCommandEncoder final : public CommandEncoder
    {
    public:
        void draw_geometry(const GeometryDrawCommand&) override {}
    };

    /// Provider that dispenses no-op encoders.
    class NullCommandEncoderProvider final : public CommandEncoderProvider
    {
    public:
        std::unique_ptr<CommandEncoder> begin_encoder(const CommandEncoderDescriptor&) override
        {
            return std::make_unique<NullCommandEncoder>();
        }

        void end_encoder(const CommandEncoderDescriptor&, std::unique_ptr<CommandEncoder>) override {}
    };
}
