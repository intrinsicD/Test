#pragma once

#include <string>
#include <vector>

#include "engine/core/telemetry/schema.hpp"
#include "engine/rendering/resources/resource_provider.hpp"

namespace engine::rendering::backend::validation
{
    struct BackendValidationObservation
    {
        std::string identifier{};
        bool passed{false};
        std::string message{};
    };

    struct BackendValidationReport
    {
        resources::GraphicsApi api{resources::GraphicsApi::Unknown};
        std::vector<BackendValidationObservation> observations{};
    };

    [[nodiscard]] BackendValidationReport validate_backend(resources::IGpuResourceProvider& provider);

    [[nodiscard]] core::telemetry::MetricSet backend_parity_metrics(const BackendValidationReport& report);
}

