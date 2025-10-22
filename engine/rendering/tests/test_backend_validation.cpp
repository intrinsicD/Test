#include <gtest/gtest.h>

#include <string>
#include <variant>

#include "engine/core/telemetry/schema.hpp"
#include "engine/rendering/backend/validation.hpp"
#include "engine/rendering/resources/recording_gpu_resource_provider.hpp"

namespace
{
    using engine::rendering::backend::validation::BackendValidationObservation;
    using engine::rendering::backend::validation::BackendValidationReport;
    using engine::rendering::resources::GraphicsApi;

    [[nodiscard]] std::string api_token(GraphicsApi api)
    {
        switch (api)
        {
        case GraphicsApi::Unknown:
            return "unknown";
        case GraphicsApi::Vulkan:
            return "vulkan";
        case GraphicsApi::DirectX12:
            return "directx12";
        case GraphicsApi::Metal:
            return "metal";
        case GraphicsApi::OpenGL:
            return "opengl";
        }
        return "unknown";
    }

    [[nodiscard]] std::int64_t find_counter(const engine::core::telemetry::MetricSet& metrics, const std::string& name)
    {
        for (std::size_t index = 0; index < metrics.descriptors.size(); ++index)
        {
            const auto& descriptor = metrics.descriptors[index];
            if (descriptor.name == name)
            {
                const auto& sample = metrics.samples.at(index);
                EXPECT_TRUE(std::holds_alternative<std::int64_t>(sample.value));
                if (!std::holds_alternative<std::int64_t>(sample.value))
                {
                    return 0;
                }
                return std::get<std::int64_t>(sample.value);
            }
        }
        ADD_FAILURE() << "Metric not found: " << name;
        return 0;
    }

    [[nodiscard]] double find_gauge(const engine::core::telemetry::MetricSet& metrics, const std::string& name)
    {
        for (std::size_t index = 0; index < metrics.descriptors.size(); ++index)
        {
            const auto& descriptor = metrics.descriptors[index];
            if (descriptor.name == name)
            {
                const auto& sample = metrics.samples.at(index);
                EXPECT_TRUE(std::holds_alternative<double>(sample.value));
                if (!std::holds_alternative<double>(sample.value))
                {
                    return 0.0;
                }
                return std::get<double>(sample.value);
            }
        }
        ADD_FAILURE() << "Metric not found: " << name;
        return 0.0;
    }
}

class BackendValidationTest : public ::testing::TestWithParam<GraphicsApi>
{
};

TEST_P(BackendValidationTest, RecordingProviderPassesAllChecks)
{
    const auto api = GetParam();
    engine::rendering::resources::RecordingGpuResourceProvider provider{api};

    const BackendValidationReport report = engine::rendering::backend::validation::validate_backend(provider);
    EXPECT_EQ(report.api, api);
    ASSERT_FALSE(report.observations.empty());

    for (const BackendValidationObservation& observation : report.observations)
    {
        EXPECT_FALSE(observation.identifier.empty());
        EXPECT_TRUE(observation.passed) << observation.identifier << ": " << observation.message;
    }

    const auto metrics = engine::rendering::backend::validation::backend_parity_metrics(report);
    ASSERT_EQ(metrics.descriptors.size(), metrics.samples.size());

    const auto api_name = api_token(api);
    const auto total_metric = "rendering.backend." + api_name + ".checks.total";
    const auto passed_metric = "rendering.backend." + api_name + ".checks.passed";
    const auto failed_metric = "rendering.backend." + api_name + ".checks.failed";

    EXPECT_EQ(find_counter(metrics, total_metric), static_cast<std::int64_t>(report.observations.size()));
    EXPECT_EQ(find_counter(metrics, passed_metric), static_cast<std::int64_t>(report.observations.size()));
    EXPECT_EQ(find_counter(metrics, failed_metric), 0);

    for (const BackendValidationObservation& observation : report.observations)
    {
        const auto gauge_name = "rendering.backend." + api_name + ".check." + observation.identifier;
        EXPECT_DOUBLE_EQ(find_gauge(metrics, gauge_name), observation.passed ? 1.0 : 0.0);
    }
}

INSTANTIATE_TEST_SUITE_P(AllBackends, BackendValidationTest,
                         ::testing::Values(GraphicsApi::Vulkan, GraphicsApi::DirectX12, GraphicsApi::Metal,
                                           GraphicsApi::OpenGL));

