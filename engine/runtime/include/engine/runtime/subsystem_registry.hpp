#pragma once

#include <functional>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "engine/core/plugin/isubsystem_interface.hpp"

namespace engine::runtime {

struct SubsystemDescriptor {
    std::string name{};
    std::vector<std::string> dependencies{};
    std::function<std::shared_ptr<engine::core::plugin::ISubsystemInterface>()> factory{};
    bool enabled_by_default{true};
};

class SubsystemRegistry {
public:
    SubsystemRegistry() = default;
    SubsystemRegistry(const SubsystemRegistry&) = default;
    SubsystemRegistry(SubsystemRegistry&&) noexcept = default;
    SubsystemRegistry& operator=(const SubsystemRegistry&) = default;
    SubsystemRegistry& operator=(SubsystemRegistry&&) noexcept = default;
    ~SubsystemRegistry() = default;

    void register_subsystem(SubsystemDescriptor descriptor);

    [[nodiscard]] bool contains(std::string_view name) const noexcept;

    [[nodiscard]] std::vector<std::string_view> registered_names() const;

    [[nodiscard]] std::vector<std::shared_ptr<engine::core::plugin::ISubsystemInterface>> load(
        std::span<const std::string_view> requested) const;

    [[nodiscard]] std::vector<std::shared_ptr<engine::core::plugin::ISubsystemInterface>> load_defaults() const;

private:
    void gather_dependencies(std::string_view name, std::unordered_set<std::string>& accumulator) const;

    std::vector<SubsystemDescriptor> descriptors_{};
    std::unordered_map<std::string, std::size_t, std::hash<std::string>, std::equal_to<>> index_map_{};
};

SubsystemRegistry make_default_subsystem_registry();

}  // namespace engine::runtime
