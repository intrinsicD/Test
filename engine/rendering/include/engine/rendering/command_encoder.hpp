#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

#include "engine/assets/handles.hpp"
#include "engine/math/matrix.hpp"
#include "engine/math/transform.hpp"
#include "engine/math/vector.hpp"
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
        engine::math::mat4 view_matrix{engine::math::identity_matrix<float, 4>()};
        engine::math::mat4 projection_matrix{engine::math::identity_matrix<float, 4>()};
        engine::math::vec3 camera_position{0.0F, 0.0F, 0.0F};
        bool has_color_override{false};
        engine::math::vec3 color_override{1.0F, 1.0F, 1.0F};
        float alpha_override{1.0F};
    };

    /// High-level compute dispatch request emitted by render or compute passes.
    struct ComputeDispatchCommand
    {
        std::uint32_t group_count_x{1};
        std::uint32_t group_count_y{1};
        std::uint32_t group_count_z{1};
    };

    /// Union of commands that can be recorded by command encoders.
    class EncodedCommand
    {
    public:
        enum class Type
        {
            DrawGeometry,
            DispatchCompute,
        };

        [[nodiscard]] static EncodedCommand make_draw(const GeometryDrawCommand& command)
        {
            return EncodedCommand{Type::DrawGeometry, command};
        }

        [[nodiscard]] static EncodedCommand make_dispatch(const ComputeDispatchCommand& command)
        {
            return EncodedCommand{Type::DispatchCompute, command};
        }

        [[nodiscard]] Type type() const noexcept { return type_; }

        [[nodiscard]] bool is_draw() const noexcept
        {
            return type_ == Type::DrawGeometry;
        }

        [[nodiscard]] bool is_dispatch() const noexcept
        {
            return type_ == Type::DispatchCompute;
        }

        [[nodiscard]] const GeometryDrawCommand& geometry_draw() const
        {
            return std::get<GeometryDrawCommand>(payload_);
        }

        [[nodiscard]] const ComputeDispatchCommand& compute_dispatch() const
        {
            return std::get<ComputeDispatchCommand>(payload_);
        }

    private:
        using Payload = std::variant<GeometryDrawCommand, ComputeDispatchCommand>;

        EncodedCommand(Type type, const GeometryDrawCommand& command)
            : type_(type), payload_(command)
        {
        }

        EncodedCommand(Type type, const ComputeDispatchCommand& command)
            : type_(type), payload_(command)
        {
        }

        Type type_;
        Payload payload_;
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

    /**
     * \brief Concrete command encoder that records draw and dispatch commands in memory.
     *
     * The recording encoder is useful for unit tests, diagnostics, and CPU-only execution
     * paths that need to inspect the commands emitted by render passes without touching a
     * backend-specific command buffer.
     */
    class RecordingCommandEncoder final : public CommandEncoder
    {
    public:
        void draw_geometry(const GeometryDrawCommand& command) override;
        void dispatch_compute(const ComputeDispatchCommand& command) override;

        /// Remove all previously recorded commands.
        void clear() noexcept;

        [[nodiscard]] const std::vector<EncodedCommand>& commands() const noexcept
        {
            return commands_;
        }

        [[nodiscard]] const std::vector<GeometryDrawCommand>& geometry_draws() const noexcept
        {
            return geometry_draws_;
        }

        [[nodiscard]] const std::vector<ComputeDispatchCommand>& compute_dispatches() const noexcept
        {
            return compute_dispatches_;
        }

    private:
        std::vector<EncodedCommand> commands_{};
        std::vector<GeometryDrawCommand> geometry_draws_{};
        std::vector<ComputeDispatchCommand> compute_dispatches_{};
    };

    /**
     * \brief Provider that hands out \ref RecordingCommandEncoder instances and tracks pass metadata.
     */
    class RecordingCommandEncoderProvider final : public CommandEncoderProvider
    {
    public:
        struct DescriptorRecord
        {
            std::string pass_name;
            QueueType queue{QueueType::Graphics};
            CommandBufferHandle command_buffer{};
        };

        [[nodiscard]] std::unique_ptr<CommandEncoder> begin_encoder(
            const CommandEncoderDescriptor& descriptor) override;

        void end_encoder(const CommandEncoderDescriptor& descriptor,
                         std::unique_ptr<CommandEncoder> encoder) override;

        /// Clear recorded begin/end descriptors and completed encoders.
        void clear() noexcept;

        [[nodiscard]] const std::vector<DescriptorRecord>& begin_records() const noexcept
        {
            return begin_records_;
        }

        [[nodiscard]] const std::vector<DescriptorRecord>& end_records() const noexcept
        {
            return end_records_;
        }

        [[nodiscard]] const std::vector<std::unique_ptr<RecordingCommandEncoder>>& completed_encoders() const noexcept
        {
            return completed_encoders_;
        }

        /// Transfer ownership of completed encoders to the caller.
        [[nodiscard]] std::vector<std::unique_ptr<RecordingCommandEncoder>> release_completed_encoders() noexcept;

    private:
        std::vector<DescriptorRecord> begin_records_{};
        std::vector<DescriptorRecord> end_records_{};
        std::vector<std::unique_ptr<RecordingCommandEncoder>> completed_encoders_{};
    };
}
