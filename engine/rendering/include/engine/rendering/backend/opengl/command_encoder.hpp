#pragma once

#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "engine/rendering/command_encoder.hpp"

namespace engine::rendering::backend::opengl
{
    class OpenGLGpuResourceProvider;

    /**
     * \brief Recorded command buffer produced by the OpenGL command encoder.
     */
    class OpenGLCommandBuffer
    {
    public:
        OpenGLCommandBuffer() = default;

        void reset(std::string_view pass_name, CommandBufferHandle handle, QueueType queue) noexcept;

        [[nodiscard]] std::string_view label() const noexcept { return label_; }
        [[nodiscard]] CommandBufferHandle handle() const noexcept { return handle_; }
        [[nodiscard]] QueueType queue() const noexcept { return queue_; }

        using EncodedCommand = engine::rendering::EncodedCommand;

        [[nodiscard]] const std::vector<EncodedCommand>& commands() const noexcept
        {
            return commands_;
        }

        void push_command(const EncodedCommand& command)
        {
            commands_.push_back(command);
        }

        void clear_commands() noexcept
        {
            commands_.clear();
        }

    private:
        std::string label_{};
        CommandBufferHandle handle_{};
        QueueType queue_{QueueType::Graphics};
        std::vector<EncodedCommand> commands_{};
    };

    /**
     * \brief Command encoder that records draw commands for OpenGL execution.
     */
    class OpenGLCommandEncoder final : public CommandEncoder
    {
    public:
        explicit OpenGLCommandEncoder(OpenGLCommandBuffer& buffer) noexcept;

        void draw_geometry(const GeometryDrawCommand& command) override;
        void dispatch_compute(const ComputeDispatchCommand& command) override;

    private:
        OpenGLCommandBuffer* buffer_;
    };

    /**
     * \brief Provider handing out OpenGL command encoders tied to native command buffers.
     */
    class OpenGLCommandEncoderProvider final : public CommandEncoderProvider
    {
    public:
        explicit OpenGLCommandEncoderProvider(OpenGLGpuResourceProvider& provider) noexcept;

        [[nodiscard]] std::unique_ptr<CommandEncoder> begin_encoder(
            const CommandEncoderDescriptor& descriptor) override;

        void end_encoder(const CommandEncoderDescriptor& descriptor,
                         std::unique_ptr<CommandEncoder> encoder) override;

    private:
        OpenGLGpuResourceProvider* provider_;
    };
}