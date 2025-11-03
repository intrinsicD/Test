#pragma once

#include <cstddef>
#include <functional>

#include "engine/rendering/presentation_backend.hpp"

namespace engine::rendering::backend::mock
{
    /**
     * \brief Headless presentation backend used for mock/windowless configurations.
     *
     * The backend records invocation metadata and forwards the runtime context to an
     * optional callback so tests or tooling can observe presentation cadence without
     * requiring a GPU device.
     */
    class MockPresentationBackend final : public PresentationBackend
    {
    public:
        using Callback = std::function<void(const RuntimePresentationContext&)>;

        explicit MockPresentationBackend(Callback callback = {});

        void present(const RuntimePresentationContext& context) override;

        void set_callback(Callback callback);

        [[nodiscard]] std::size_t invocation_count() const noexcept;

        [[nodiscard]] double last_delta_seconds() const noexcept;

    private:
        Callback callback_{};
        std::size_t invocation_count_{0};
        double last_delta_seconds_{0.0};
    };
}
