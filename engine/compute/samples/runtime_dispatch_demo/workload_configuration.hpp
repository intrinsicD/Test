#pragma once

#include <cstddef>
#include <string_view>

namespace engine::runtime
{
    class RuntimeHost;
}

namespace engine::compute::samples
{

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

    void configure_runtime_host(engine::runtime::RuntimeHost& host, WorkloadProfile workload);

}  // namespace engine::compute::samples

