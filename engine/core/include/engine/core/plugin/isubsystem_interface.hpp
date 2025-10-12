#pragma once

#include <span>
#include <string_view>

namespace engine::core::plugin {

struct SubsystemLifecycleContext {
    std::string_view runtime_name{};
};

struct SubsystemUpdateContext {
    double delta_time{0.0};
};

class ISubsystemInterface {
public:
    virtual ~ISubsystemInterface() = default;

    [[nodiscard]] virtual std::string_view name() const noexcept = 0;

    [[nodiscard]] virtual std::span<const std::string_view> dependencies() const noexcept = 0;

    virtual void initialize(const SubsystemLifecycleContext& context) = 0;

    virtual void shutdown(const SubsystemLifecycleContext& context) noexcept = 0;

    virtual void tick(const SubsystemUpdateContext& context) = 0;
};

}  // namespace engine::core::plugin
