#pragma once

#include <functional>
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
        explicit RenderPass(std::string name);
        virtual ~RenderPass() = default;

        [[nodiscard]] std::string_view name() const noexcept;

        /// Describe the resources that this pass will access.
        virtual void setup(FrameGraphPassBuilder& builder) = 0;

        /// Execute the pass using the inputs prepared by the frame-graph.
        virtual void execute(FrameGraphPassExecutionContext& context) = 0;

    private:
        std::string name_;
    };

    /**
     * \brief Convenience render pass that accepts lambdas for setup and execute.
     */
    class CallbackRenderPass final : public RenderPass
    {
    public:
        using SetupFunction = std::function<void(FrameGraphPassBuilder&)>;
        using ExecuteFunction = std::function<void(FrameGraphPassExecutionContext&)>;

        CallbackRenderPass(std::string name, SetupFunction setup, ExecuteFunction execute);

        void setup(FrameGraphPassBuilder& builder) override;
        void execute(FrameGraphPassExecutionContext& context) override;

    private:
        SetupFunction setup_;
        ExecuteFunction execute_;
    };

    inline RenderPass::RenderPass(std::string name) : name_(std::move(name))
    {
    }

    inline std::string_view RenderPass::name() const noexcept
    {
        return name_;
    }

    inline CallbackRenderPass::CallbackRenderPass(std::string name, SetupFunction setup,
                                                  ExecuteFunction execute)
        : RenderPass(std::move(name)), setup_(std::move(setup)), execute_(std::move(execute))
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
