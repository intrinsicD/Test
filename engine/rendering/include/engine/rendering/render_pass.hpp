#pragma once

#include <functional>
#include <ostream>
#include <string>
#include <string_view>
#include <utility>

#include "engine/assets/handles.hpp"
#include "engine/rendering/gpu_scheduler.hpp"
#include "engine/rendering/resources/resource_provider.hpp"

namespace engine::scene
{
    class Scene;
}

namespace engine::rendering
{
    class FrameGraphPassBuilder;
    struct FrameGraphPassExecutionContext;
    class MaterialSystem;
    class CommandEncoder;
    class CommandEncoderProvider;

    /// High-level lifecycle stage associated with a render pass.
    enum class PassPhase
    {
        Unknown,
        Setup,
        Geometry,
        Lighting,
        PostProcess,
        Compute,
        Transfer,
        Presentation,
    };

    inline std::ostream& operator<<(std::ostream& os, PassPhase phase)
    {
        switch (phase)
        {
        case PassPhase::Unknown:
            return os << "Unknown";
        case PassPhase::Setup:
            return os << "Setup";
        case PassPhase::Geometry:
            return os << "Geometry";
        case PassPhase::Lighting:
            return os << "Lighting";
        case PassPhase::PostProcess:
            return os << "PostProcess";
        case PassPhase::Compute:
            return os << "Compute";
        case PassPhase::Transfer:
            return os << "Transfer";
        case PassPhase::Presentation:
            return os << "Presentation";
        }
        return os;
    }

    /// Severity attached to validation diagnostics emitted by a pass.
    enum class ValidationSeverity
    {
        Info,
        Warning,
        Error,
    };

    inline std::ostream& operator<<(std::ostream& os, ValidationSeverity severity)
    {
        switch (severity)
        {
        case ValidationSeverity::Info:
            return os << "Info";
        case ValidationSeverity::Warning:
            return os << "Warning";
        case ValidationSeverity::Error:
            return os << "Error";
        }
        return os;
    }

    /**
     * \brief Interface exposed by the platform layer to satisfy GPU resource requests.
     */
    class RenderResourceProvider
    {
    public:
        virtual ~RenderResourceProvider() = default;

        /// Ensure that the mesh identified by \p handle is resident on the GPU.
        virtual void require_mesh(const engine::assets::MeshHandle& handle) = 0;

        /// Ensure that the graph identified by \p handle is resident on the GPU.
        virtual void require_graph(const engine::assets::GraphHandle& handle) = 0;

        /// Ensure that the point cloud identified by \p handle is resident on the GPU.
        virtual void require_point_cloud(const engine::assets::PointCloudHandle& handle) = 0;

        /// Ensure that the material identified by \p handle is ready for use.
        virtual void require_material(const engine::assets::MaterialHandle& handle) = 0;

        /// Ensure that the shader program identified by \p handle is compiled and boundable.
        virtual void require_shader(const engine::assets::ShaderHandle& handle) = 0;
    };

    /**
     * \brief Lightweight description of the scene subset to be rendered.
     */
    struct RenderView
    {
        scene::Scene& scene;
    };

    /**
     * \brief Context passed to render passes during execution.
     */
    struct RenderExecutionContext
    {
        RenderResourceProvider& resources;
        MaterialSystem& materials;
        RenderView view;
        IGpuScheduler& scheduler;
        resources::IGpuResourceProvider& device_resources;
        CommandEncoderProvider& encoders;
    };

    /**
     * \brief Abstract base class implemented by all render passes.
     */
    class RenderPass
    {
    public:
        explicit RenderPass(std::string name, QueueType queue = QueueType::Graphics,
                            PassPhase phase = PassPhase::Unknown,
                            ValidationSeverity validation = ValidationSeverity::Info);
        virtual ~RenderPass() = default;

        [[nodiscard]] std::string_view name() const noexcept;
        void set_queue(QueueType queue) noexcept;
        [[nodiscard]] QueueType queue() const noexcept;
        void set_phase(PassPhase phase) noexcept;
        [[nodiscard]] PassPhase phase() const noexcept;
        void set_validation_severity(ValidationSeverity severity) noexcept;
        [[nodiscard]] ValidationSeverity validation_severity() const noexcept;

        /// Describe the resources that this pass will access.
        virtual void setup(FrameGraphPassBuilder& builder) = 0;

        /// Execute the pass using the inputs prepared by the frame-graph.
        virtual void execute(FrameGraphPassExecutionContext& context) = 0;

    private:
        std::string name_;
        QueueType queue_{QueueType::Graphics};
        PassPhase phase_{PassPhase::Unknown};
        ValidationSeverity validation_{ValidationSeverity::Info};
    };

    /**
     * \brief Convenience render pass that accepts lambdas for setup and execute.
     */
    class CallbackRenderPass final : public RenderPass
    {
    public:
        using SetupFunction = std::function<void(FrameGraphPassBuilder&)>;
        using ExecuteFunction = std::function<void(FrameGraphPassExecutionContext&)>;

        CallbackRenderPass(std::string name, SetupFunction setup, ExecuteFunction execute,
                           QueueType queue = QueueType::Graphics,
                           PassPhase phase = PassPhase::Unknown,
                           ValidationSeverity validation = ValidationSeverity::Info);

        void setup(FrameGraphPassBuilder& builder) override;
        void execute(FrameGraphPassExecutionContext& context) override;

    private:
        SetupFunction setup_;
        ExecuteFunction execute_;
    };

    inline RenderPass::RenderPass(std::string name, QueueType queue, PassPhase phase,
                                  ValidationSeverity validation)
        : name_(std::move(name)), queue_(queue), phase_(phase), validation_(validation)
    {
    }

    inline std::string_view RenderPass::name() const noexcept
    {
        return name_;
    }

    inline void RenderPass::set_queue(QueueType queue) noexcept
    {
        queue_ = queue;
    }

    inline QueueType RenderPass::queue() const noexcept
    {
        return queue_;
    }

    inline void RenderPass::set_phase(PassPhase phase) noexcept
    {
        phase_ = phase;
    }

    inline PassPhase RenderPass::phase() const noexcept
    {
        return phase_;
    }

    inline void RenderPass::set_validation_severity(ValidationSeverity severity) noexcept
    {
        validation_ = severity;
    }

    inline ValidationSeverity RenderPass::validation_severity() const noexcept
    {
        return validation_;
    }

    inline CallbackRenderPass::CallbackRenderPass(std::string name, SetupFunction setup,
                                                  ExecuteFunction execute, QueueType queue,
                                                  PassPhase phase, ValidationSeverity validation)
        : RenderPass(std::move(name), queue, phase, validation),
          setup_(std::move(setup)),
          execute_(std::move(execute))
    {
    }

    inline void CallbackRenderPass::setup(FrameGraphPassBuilder& builder)
    {
        if (setup_)
        {
            setup_(builder);
        }
    }

    inline void CallbackRenderPass::execute(FrameGraphPassExecutionContext& context)
    {
        if (execute_)
        {
            execute_(context);
        }
    }
} // namespace engine::rendering
