#include "engine/rendering/backend/validation.hpp"

#include <algorithm>
#include <array>
#include <stdexcept>
#include <string>
#include <utility>

#include "engine/rendering/gpu_scheduler.hpp"

namespace engine::rendering::backend::validation
{
    namespace
    {
        class CommandBufferGuard
        {
        public:
            CommandBufferGuard(resources::IGpuResourceProvider& provider, CommandBufferHandle handle) noexcept
                : provider_{provider}, handle_{handle}
            {
            }

            CommandBufferGuard(const CommandBufferGuard&) = delete;
            CommandBufferGuard& operator=(const CommandBufferGuard&) = delete;

            CommandBufferGuard(CommandBufferGuard&& other) noexcept
                : provider_{other.provider_}, handle_{other.handle_}, active_{other.active_}
            {
                other.active_ = false;
            }

            CommandBufferGuard& operator=(CommandBufferGuard&& other) noexcept
            {
                if (this != &other)
                {
                    release();
                    provider_ = other.provider_;
                    handle_ = other.handle_;
                    active_ = other.active_;
                    other.active_ = false;
                }
                return *this;
            }

            ~CommandBufferGuard()
            {
                release();
            }

        private:
            void release() noexcept
            {
                if (active_)
                {
                    provider_.recycle_command_buffer(handle_);
                    active_ = false;
                }
            }

            resources::IGpuResourceProvider& provider_;
            CommandBufferHandle handle_{};
            bool active_{true};
        };

        [[nodiscard]] std::string_view api_token(resources::GraphicsApi api) noexcept
        {
            using resources::GraphicsApi;
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

        template <typename Function>
        void record_check(BackendValidationReport& report, std::string identifier, Function&& function)
        {
            BackendValidationObservation observation{};
            observation.identifier = std::move(identifier);
            try
            {
                function();
                observation.passed = true;
            }
            catch (const std::exception& exception)
            {
                observation.passed = false;
                observation.message = exception.what();
            }
            catch (...)
            {
                observation.passed = false;
                observation.message = "unknown failure";
            }
            report.observations.push_back(std::move(observation));
        }

        [[nodiscard]] std::string make_queue_identifier(std::string_view prefix, std::string_view queue)
        {
            std::string identifier;
            identifier.reserve(prefix.size() + queue.size() + 1U);
            identifier.append(prefix);
            identifier.push_back('.');
            identifier.append(queue);
            return identifier;
        }
    } // namespace

    BackendValidationReport validate_backend(resources::IGpuResourceProvider& provider)
    {
        BackendValidationReport report{};
        report.api = provider.api();

        bool frame_started = false;
        record_check(report, "lifecycle.begin_frame", [&]
        {
            provider.begin_frame();
            frame_started = true;
        });

        const std::array queues{QueueType::Graphics, QueueType::Compute, QueueType::Transfer};
        const std::array queue_names{
            std::string_view{"graphics"}, std::string_view{"compute"},
            std::string_view{"transfer"}
        };
        std::size_t next_handle_index = 0;

        for (std::size_t index = 0; index < queues.size(); ++index)
        {
            const auto queue = queues[index];
            const auto name = queue_names[index];
            record_check(report, make_queue_identifier("queue", name), [&]
            {
                const auto handle = provider.queue_handle(queue);
                if (handle.api != report.api)
                {
                    throw std::runtime_error{"queue handle API mismatch"};
                }
                if (handle.queue != queue)
                {
                    throw std::runtime_error{"queue handle type mismatch"};
                }
                if (handle.value == 0U)
                {
                    throw std::runtime_error{"queue handle returned zero value"};
                }
            });

            record_check(report, make_queue_identifier("command_buffer", name), [&]
            {
                CommandBufferHandle handle{++next_handle_index};
                const auto native = provider.allocate_command_buffer(queue, "backend_validation", handle);
                CommandBufferGuard guard{provider, handle};
                if (native.api != report.api)
                {
                    throw std::runtime_error{"command buffer API mismatch"};
                }
                if (native.queue != queue)
                {
                    throw std::runtime_error{"command buffer queue mismatch"};
                }
                if (native.index != handle.index)
                {
                    throw std::runtime_error{"command buffer index mismatch"};
                }
                if (native.value == 0U)
                {
                    throw std::runtime_error{"command buffer returned zero value"};
                }
            });
        }

        record_check(report, "lifecycle.end_frame", [&]
        {
            if (!frame_started)
            {
                throw std::runtime_error{"begin_frame failed; end_frame skipped"};
            }
            provider.end_frame();
            frame_started = false;
        });

        record_check(report, "fence.resolve", [&]
        {
            resources::Fence fence{"backend_validation_fence"};
            const auto native = provider.resolve_fence(fence);
            if (native.api != report.api)
            {
                throw std::runtime_error{"fence API mismatch"};
            }
            if (native.value == 0U)
            {
                throw std::runtime_error{"fence returned zero value"};
            }
        });

        record_check(report, "semaphore.resolve", [&]
        {
            resources::TimelineSemaphore semaphore{"backend_validation_semaphore"};
            const auto native = provider.resolve_semaphore(semaphore);
            if (native.api != report.api)
            {
                throw std::runtime_error{"timeline semaphore API mismatch"};
            }
            if (native.value == 0U)
            {
                throw std::runtime_error{"timeline semaphore returned zero value"};
            }
        });

        return report;
    }

    core::telemetry::MetricSet backend_parity_metrics(const BackendValidationReport& report)
    {
        core::telemetry::MetricSet metrics{};
        metrics.descriptors.reserve(3U + report.observations.size());
        metrics.samples.reserve(3U + report.observations.size());

        const auto api_name = std::string{api_token(report.api)};
        const auto total_checks = static_cast<std::int64_t>(report.observations.size());
        const auto failed_checks = static_cast<std::int64_t>(std::count_if(
            report.observations.begin(), report.observations.end(), [](const BackendValidationObservation& observation)
            {
                return !observation.passed;
            }));
        const auto passed_checks = total_checks - failed_checks;

        const auto add_counter = [&](std::string name, std::string_view description, std::int64_t value)
        {
            const std::size_t index = metrics.descriptors.size();
            core::telemetry::MetricDescriptor descriptor{};
            descriptor.name = std::move(name);
            descriptor.kind = core::telemetry::MetricKind::Counter;
            descriptor.unit = core::telemetry::MetricUnit::Count;
            descriptor.description.assign(description);
            metrics.descriptors.push_back(std::move(descriptor));

            core::telemetry::MetricSample sample{};
            sample.descriptor_index = index;
            sample.value = value;
            metrics.samples.push_back(std::move(sample));
        };

        const auto add_gauge = [&](std::string name, std::string_view description, double value)
        {
            const std::size_t index = metrics.descriptors.size();
            core::telemetry::MetricDescriptor descriptor{};
            descriptor.name = std::move(name);
            descriptor.kind = core::telemetry::MetricKind::Gauge;
            descriptor.unit = core::telemetry::MetricUnit::None;
            descriptor.description.assign(description);
            metrics.descriptors.push_back(std::move(descriptor));

            core::telemetry::MetricSample sample{};
            sample.descriptor_index = index;
            sample.value = value;
            metrics.samples.push_back(std::move(sample));
        };

        add_counter("rendering.backend." + api_name + ".checks.total",
                    "Total backend validation checks executed", total_checks);
        add_counter("rendering.backend." + api_name + ".checks.passed",
                    "Backend validation checks that passed", passed_checks);
        add_counter("rendering.backend." + api_name + ".checks.failed",
                    "Backend validation checks that failed", failed_checks);

        for (const auto& observation : report.observations)
        {
            add_gauge("rendering.backend." + api_name + ".check." + observation.identifier,
                      "Backend validation status", observation.passed ? 1.0 : 0.0);
        }

        return metrics;
    }
}