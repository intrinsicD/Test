#include "engine/rendering/backend/vulkan/command_encoder.hpp"

#include <stdexcept>

#include "engine/rendering/backend/vulkan/resource_provider.hpp"

namespace engine::rendering::backend::vulkan
{
    void VulkanCommandBuffer::reset(std::string_view pass_name, CommandBufferHandle handle, QueueType queue) noexcept
    {
        label_.assign(pass_name.begin(), pass_name.end());
        handle_ = handle;
        queue_ = queue;
        clear_commands();
    }

    VulkanCommandEncoder::VulkanCommandEncoder(VulkanCommandBuffer& buffer) noexcept : buffer_(&buffer) {}

    void VulkanCommandEncoder::draw_geometry(const GeometryDrawCommand& command)
    {
        if (buffer_ == nullptr)
        {
            throw std::runtime_error{"VulkanCommandEncoder invoked without active command buffer"};
        }
        buffer_->push_command(VulkanCommandBuffer::EncodedCommand::make_draw(command));
    }

    void VulkanCommandEncoder::dispatch_compute(const ComputeDispatchCommand& command)
    {
        if (buffer_ == nullptr)
        {
            throw std::runtime_error{"VulkanCommandEncoder invoked without active command buffer"};
        }
        buffer_->push_command(VulkanCommandBuffer::EncodedCommand::make_dispatch(command));
    }

    VulkanCommandEncoderProvider::VulkanCommandEncoderProvider(VulkanGpuResourceProvider& provider) noexcept
        : provider_(&provider)
    {
    }

    std::unique_ptr<CommandEncoder> VulkanCommandEncoderProvider::begin_encoder(
        const CommandEncoderDescriptor& descriptor)
    {
        if (provider_ == nullptr)
        {
            throw std::runtime_error{"VulkanCommandEncoderProvider has no resource provider"};
        }

        auto* buffer = provider_->command_buffer(descriptor.command_buffer);
        if (buffer == nullptr)
        {
            throw std::runtime_error{"VulkanCommandEncoderProvider could not resolve command buffer"};
        }

        buffer->reset(descriptor.pass_name, descriptor.command_buffer, descriptor.queue);
        return std::make_unique<VulkanCommandEncoder>(*buffer);
    }

    void VulkanCommandEncoderProvider::end_encoder(const CommandEncoderDescriptor& descriptor,
                                                   std::unique_ptr<CommandEncoder> encoder)
    {
        static_cast<void>(descriptor);
        static_cast<void>(encoder);
    }
}

