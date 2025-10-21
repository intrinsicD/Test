#pragma once

#include <span>
#include <string_view>

namespace engine::core::plugin {

struct SubsystemLifecycleContext {
    /// Human-readable identifier for the runtime instance requesting lifecycle events.
    ///
    /// When invoked by the runtime host this maps to the active scene name. Plugins
    /// must treat the value as non-owning and avoid caching dangling references.
    std::string_view runtime_name{};
};

struct SubsystemUpdateContext {
    /// Simulation timestep in seconds supplied to `tick`.
    double delta_time{0.0};
};

/// Subsystems discovered at runtime implement this interface to integrate with the
/// host lifecycle. Implementations must be deterministic and re-entrant: `initialize`
/// may run multiple times across the process lifetime, and `shutdown` is always
/// invoked in reverse registration order whenever initialization succeeds.
class ISubsystemInterface {
public:
    virtual ~ISubsystemInterface() = default;

    [[nodiscard]] virtual std::string_view name() const noexcept = 0;

    [[nodiscard]] virtual std::span<const std::string_view> dependencies() const noexcept = 0;

    /// Prepare the subsystem for use. Implementations may throw to signal unrecoverable
    /// startup failures; the runtime host guarantees previously initialized subsystems are
    /// shut down before the exception propagates.
    virtual void initialize(const SubsystemLifecycleContext& context) = 0;

    /// Tear down the subsystem. Must be noexcept and idempotent because the runtime host
    /// will invoke `shutdown` when initialization fails partway through and again during
    /// normal teardown.
    virtual void shutdown(const SubsystemLifecycleContext& context) noexcept = 0;

    virtual void tick(const SubsystemUpdateContext& context) = 0;
};

}  // namespace engine::core::plugin
