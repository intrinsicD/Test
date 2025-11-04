#include "engine/rendering/backend/mock/presentation_backend.hpp"

#include <utility>

namespace engine::rendering::backend::mock
{
    MockPresentationBackend::MockPresentationBackend(Callback callback)
        : callback_(std::move(callback))
    {
    }

    void MockPresentationBackend::present(const RuntimePresentationContext& context)
    {
        ++invocation_count_;
        last_delta_seconds_ = context.delta_seconds;
        if (callback_)
        {
            callback_(context);
        }
    }

    void MockPresentationBackend::set_callback(Callback callback)
    {
        callback_ = std::move(callback);
    }

    std::size_t MockPresentationBackend::invocation_count() const noexcept
    {
        return invocation_count_;
    }

    double MockPresentationBackend::last_delta_seconds() const noexcept
    {
        return last_delta_seconds_;
    }
}
