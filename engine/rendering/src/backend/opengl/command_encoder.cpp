#include "engine/rendering/backend/opengl/command_encoder.hpp"

#include <stdexcept>

#include "engine/rendering/backend/opengl/resource_provider.hpp"

namespace engine::rendering::backend::opengl
{
    void OpenGLCommandBuffer::reset(std::string_view pass_name, CommandBufferHandle handle, QueueType queue) noexcept
    {
        label_.assign(pass_name.data(), pass_name.size());
        handle_ = handle;
        queue_ = queue;
        draws.clear();
    }

    OpenGLCommandEncoder::OpenGLCommandEncoder(OpenGLCommandBuffer& buffer) noexcept : buffer_(&buffer) {}

    void OpenGLCommandEncoder::draw_geometry(const GeometryDrawCommand& command)
    {
        if (buffer_ == nullptr)
        {
            throw std::runtime_error{"OpenGLCommandEncoder invoked without active command buffer"};
        }
        buffer_->draws.push_back(command);
    }

    OpenGLCommandEncoderProvider::OpenGLCommandEncoderProvider(OpenGLGpuResourceProvider& provider) noexcept
        : provider_(&provider)
    {
    }

    std::unique_ptr<CommandEncoder> OpenGLCommandEncoderProvider::begin_encoder(
        const CommandEncoderDescriptor& descriptor)
    {
        if (provider_ == nullptr)
        {
            throw std::runtime_error{"OpenGLCommandEncoderProvider has no resource provider"};
        }

        auto* buffer = provider_->command_buffer(descriptor.command_buffer);
        if (buffer == nullptr)
        {
            throw std::runtime_error{"OpenGLCommandEncoderProvider could not resolve command buffer"};
        }

        buffer->reset(descriptor.pass_name, descriptor.command_buffer, descriptor.queue);
        return std::make_unique<OpenGLCommandEncoder>(*buffer);
    }

    void OpenGLCommandEncoderProvider::end_encoder(const CommandEncoderDescriptor& descriptor,
                                                   std::unique_ptr<CommandEncoder> encoder)
    {
        static_cast<void>(descriptor);
        static_cast<void>(encoder);
    }
}

