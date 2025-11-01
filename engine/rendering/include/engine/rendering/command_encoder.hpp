#pragma once

#include <cstdint>
#include <memory>
#include <string_view>

#include "engine/assets/handles.hpp"
#include "engine/math/transform.hpp"
#include "engine/rendering/components.hpp"
#include "engine/rendering/gpu_scheduler.hpp"

namespace engine::rendering
{
    /// High-level draw request emitted by render passes.
    struct GeometryDrawCommand
    {
        components::RenderGeometry::Geometry geometry;
        engine::assets::MaterialHandle material;
        engine::math::Transform<float> transform;
    };

    /// High-level compute dispatch request emitted by render or compute passes.
    struct ComputeDispatchCommand
    {
        std::uint32_t group_count_x{1};
        std::uint32_t group_count_y{1};
        std::uint32_t group_count_z{1};
    };

    /// Union of commands that can be recorded by command encoders.
    struct EncodedCommand
    {
        enum class Type
        {
            DrawGeometry,
            DispatchCompute,
        };

        Type type{Type::DrawGeometry};
        GeometryDrawCommand draw{};
        ComputeDispatchCommand dispatch{};

        [[nodiscard]] static EncodedCommand make_draw(const GeometryDrawCommand& command)
        {
            EncodedCommand encoded{};
            encoded.type = Type::DrawGeometry;
            encoded.draw = command;
            return encoded;
        }

        [[nodiscard]] static EncodedCommand make_dispatch(const ComputeDispatchCommand& command)
        {
            EncodedCommand encoded{};
            encoded.type = Type::DispatchCompute;
            encoded.dispatch = command;
            return encoded;
        }
    };

    /// Descriptor used when acquiring a command encoder for a render pass.
    struct CommandEncoderDescriptor
    {
        std::string_view pass_name;
        QueueType queue{QueueType::Graphics};
        CommandBufferHandle command_buffer{};
    };

    /**
     * \brief Interface used by render passes to record GPU work.
     */
    class CommandEncoder
    {
    public:
        virtual ~CommandEncoder() = default;

        /// Submit a geometry draw call to the underlying command buffer.
        virtual void draw_geometry(const GeometryDrawCommand& command) = 0;

        /// Submit a compute dispatch to the underlying command buffer.
        virtual void dispatch_compute(const ComputeDispatchCommand& command) = 0;
    };

    /**
     * \brief Factory that hands out command encoders tied to frame-graph submissions.
     */
    class CommandEncoderProvider
    {
    public:
        virtual ~CommandEncoderProvider() = default;

        /// Begin encoding for the render pass described by \p descriptor.
        [[nodiscard]] virtual std::unique_ptr<CommandEncoder> begin_encoder(
            const CommandEncoderDescriptor& descriptor) = 0;

        /// Finalise encoding for the render pass described by \p descriptor.
        virtual void end_encoder(const CommandEncoderDescriptor& descriptor,
                                 std::unique_ptr<CommandEncoder> encoder) = 0;
    };
}