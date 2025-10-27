#pragma once

#include <cstddef>
#include <functional>
#include <memory>
#include <string_view>

namespace engine::runtime
{
    class RuntimeHost;
}

namespace engine::compute
{
    class Dispatcher;
}

namespace engine::compute::samples
{
    using DispatcherFactory = std::function<std::unique_ptr<engine::compute::Dispatcher>()>;

    enum class WorkloadProfile
    {
        Light,
        Balanced,
        Heavy,
    };

    struct WorkloadProfileDefinition
    {
        WorkloadProfile profile{WorkloadProfile::Balanced};
        std::size_t mesh_subdivisions{32U};
        std::size_t physics_bodies{32U};
    };

    [[nodiscard]] std::string_view workload_to_string(WorkloadProfile workload) noexcept;

    [[nodiscard]] WorkloadProfileDefinition workload_definition(WorkloadProfile profile) noexcept;

    void configure_runtime_host(
        engine::runtime::RuntimeHost& host,
        WorkloadProfile workload,
        DispatcherFactory dispatcher_factory = {});
} // namespace engine::compute::samples