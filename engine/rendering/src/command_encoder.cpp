#include "engine/rendering/command_encoder.hpp"

#include <iterator>
#include <memory>
#include <utility>

namespace engine::rendering
{
    void RecordingCommandEncoder::draw_geometry(const GeometryDrawCommand& command)
    {
        commands_.push_back(EncodedCommand::make_draw(command));
        geometry_draws_.push_back(command);
    }

    void RecordingCommandEncoder::dispatch_compute(const ComputeDispatchCommand& command)
    {
        commands_.push_back(EncodedCommand::make_dispatch(command));
        compute_dispatches_.push_back(command);
    }

    void RecordingCommandEncoder::clear() noexcept
    {
        commands_.clear();
        geometry_draws_.clear();
        compute_dispatches_.clear();
    }

    std::unique_ptr<CommandEncoder> RecordingCommandEncoderProvider::begin_encoder(
        const CommandEncoderDescriptor& descriptor)
    {
        begin_records_.push_back(DescriptorRecord{
            std::string{descriptor.pass_name}, descriptor.queue, descriptor.command_buffer
        });

        return std::make_unique<RecordingCommandEncoder>();
    }

    void RecordingCommandEncoderProvider::end_encoder(const CommandEncoderDescriptor& descriptor,
                                                      std::unique_ptr<CommandEncoder> encoder)
    {
        end_records_.push_back(DescriptorRecord{
            std::string{descriptor.pass_name}, descriptor.queue, descriptor.command_buffer
        });

        if (auto* recording = dynamic_cast<RecordingCommandEncoder*>(encoder.get()); recording != nullptr)
        {
            static_cast<void>(encoder.release());
            completed_encoders_.emplace_back(recording);
        }
    }

    void RecordingCommandEncoderProvider::clear() noexcept
    {
        begin_records_.clear();
        end_records_.clear();
        completed_encoders_.clear();
    }

    std::vector<std::unique_ptr<RecordingCommandEncoder>>
    RecordingCommandEncoderProvider::release_completed_encoders() noexcept
    {
        std::vector<std::unique_ptr<RecordingCommandEncoder>> encoders{};
        encoders.reserve(completed_encoders_.size());
        std::move(completed_encoders_.begin(), completed_encoders_.end(), std::back_inserter(encoders));
        completed_encoders_.clear();
        return encoders;
    }
}
