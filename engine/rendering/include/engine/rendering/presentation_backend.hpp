#pragma once

namespace engine::runtime
{
    class RuntimeHost;
}

namespace engine::rendering
{
    /**
     * \brief Context forwarded to presentation backends when dispatching a frame.
     */
    struct RuntimePresentationContext
    {
        runtime::RuntimeHost& host;
        double delta_seconds{0.0};
    };

    /**
     * \brief Interface that consumes frame outputs and performs presentation work.
     */
    class PresentationBackend
    {
    public:
        virtual ~PresentationBackend() = default;

        /// Present the current frame using the supplied runtime context.
        virtual void present(const RuntimePresentationContext& context) = 0;
    };
}
