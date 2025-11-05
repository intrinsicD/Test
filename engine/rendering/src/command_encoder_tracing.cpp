#include "engine/rendering/command_encoder_tracing.hpp"

#include <utility>

namespace engine::rendering
{
    TracingCommandEncoder::TracingCommandEncoder(std::unique_ptr<CommandEncoder> inner,
                                                 EncoderTraceRecord& record) noexcept
        : inner_(std::move(inner))
        , record_(&record)
    {
    }

    void TracingCommandEncoder::draw_geometry(const GeometryDrawCommand& command)
    {
        if (record_ != nullptr)
        {
            record_->draw_count += 1U;
        }
        inner_->draw_geometry(command);
    }

    void TracingCommandEncoder::dispatch_compute(const ComputeDispatchCommand& command)
    {
        if (record_ != nullptr)
        {
            record_->dispatch_count += 1U;
        }
        inner_->dispatch_compute(command);
    }

    std::unique_ptr<CommandEncoder> TracingCommandEncoder::release() noexcept
    {
        record_ = nullptr;
        return std::move(inner_);
    }

    TracingCommandEncoderProvider::TracingCommandEncoderProvider(CommandEncoderProvider& inner) noexcept
        : inner_(&inner)
    {
    }

    std::unique_ptr<CommandEncoder> TracingCommandEncoderProvider::begin_encoder(
        const CommandEncoderDescriptor& descriptor)
    {
        if (inner_ == nullptr)
        {
            return nullptr;
        }

        auto inner_encoder = inner_->begin_encoder(descriptor);
        if (inner_encoder == nullptr)
        {
            return nullptr;
        }

        EncoderTraceRecord record{};
        record.pass_name.assign(descriptor.pass_name.begin(), descriptor.pass_name.end());
        record.queue = descriptor.queue;
        record.command_buffer = descriptor.command_buffer;
        records_.push_back(std::move(record));

        auto& stored_record = records_.back();
        return std::make_unique<TracingCommandEncoder>(std::move(inner_encoder), stored_record);
    }

    void TracingCommandEncoderProvider::end_encoder(const CommandEncoderDescriptor& descriptor,
                                                    std::unique_ptr<CommandEncoder> encoder)
    {
        if (inner_ == nullptr)
        {
            return;
        }

        if (auto* tracing = dynamic_cast<TracingCommandEncoder*>(encoder.get()); tracing != nullptr)
        {
            encoder = tracing->release();
        }

        inner_->end_encoder(descriptor, std::move(encoder));
    }

    std::vector<EncoderTraceRecord> TracingCommandEncoderProvider::consume_records() noexcept
    {
        return std::exchange(records_, {});
    }
} // namespace engine::rendering

