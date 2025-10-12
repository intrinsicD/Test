#include "engine/runtime/subsystem_registry.hpp"

#include <algorithm>
#include <initializer_list>
#include <stdexcept>

#if ENGINE_ENABLE_ANIMATION
#    include "engine/animation/api.hpp"
#endif
#if ENGINE_ENABLE_ASSETS
#    include "engine/assets/api.hpp"
#endif
#if ENGINE_ENABLE_COMPUTE
#    include "engine/compute/api.hpp"
#endif
#if ENGINE_ENABLE_COMPUTE_CUDA
#    include "engine/compute/cuda/api.hpp"
#endif
#if ENGINE_ENABLE_CORE
#    include "engine/core/api.hpp"
#endif
#if ENGINE_ENABLE_GEOMETRY
#    include "engine/geometry/api.hpp"
#endif
#if ENGINE_ENABLE_IO
#    include "engine/io/api.hpp"
#endif
#if ENGINE_ENABLE_PHYSICS
#    include "engine/physics/api.hpp"
#endif
#if ENGINE_ENABLE_PLATFORM
#    include "engine/platform/api.hpp"
#endif
#if ENGINE_ENABLE_RENDERING
#    include "engine/rendering/api.hpp"
#endif
#if ENGINE_ENABLE_SCENE
#    include "engine/scene/api.hpp"
#endif

namespace engine::runtime {

namespace {

class StaticSubsystem final : public engine::core::plugin::ISubsystemInterface {
public:
    StaticSubsystem(std::string_view name, std::vector<std::string_view> dependencies)
        : name_(name), dependencies_(std::move(dependencies))
    {
    }

    [[nodiscard]] std::string_view name() const noexcept override
    {
        return name_;
    }

    [[nodiscard]] std::span<const std::string_view> dependencies() const noexcept override
    {
        return dependencies_;
    }

    void initialize(const engine::core::plugin::SubsystemLifecycleContext&) override {}

    void shutdown(const engine::core::plugin::SubsystemLifecycleContext&) noexcept override {}

    void tick(const engine::core::plugin::SubsystemUpdateContext&) override {}

private:
    std::string_view name_{};
    std::vector<std::string_view> dependencies_{};
};

std::shared_ptr<engine::core::plugin::ISubsystemInterface> make_static_plugin(
    std::string_view name,
    std::initializer_list<std::string_view> dependencies = {})
{
    return std::make_shared<StaticSubsystem>(name, std::vector<std::string_view>{dependencies});
}

}  // namespace

void SubsystemRegistry::register_subsystem(SubsystemDescriptor descriptor)
{
    if (descriptor.name.empty())
    {
        throw std::invalid_argument{"Subsystem name must not be empty"};
    }
    if (!descriptor.factory)
    {
        throw std::invalid_argument{"Subsystem factory must not be null"};
    }

    const auto it = index_map_.find(descriptor.name);
    if (it != index_map_.end())
    {
        descriptors_[it->second] = std::move(descriptor);
        return;
    }

    const auto index = descriptors_.size();
    index_map_.emplace(descriptor.name, index);
    descriptors_.push_back(std::move(descriptor));
}

bool SubsystemRegistry::contains(std::string_view name) const noexcept
{
    return index_map_.find(name) != index_map_.end();
}

std::vector<std::string_view> SubsystemRegistry::registered_names() const
{
    std::vector<std::string_view> names{};
    names.reserve(descriptors_.size());
    for (const auto& descriptor : descriptors_)
    {
        names.push_back(descriptor.name);
    }
    return names;
}

void SubsystemRegistry::gather_dependencies(std::string_view name, std::unordered_set<std::string>& accumulator) const
{
    const auto it = index_map_.find(name);
    if (it == index_map_.end())
    {
        return;
    }

    const auto& descriptor = descriptors_[it->second];
    if (!accumulator.insert(descriptor.name).second)
    {
        return;
    }

    for (const auto& dependency : descriptor.dependencies)
    {
        gather_dependencies(dependency, accumulator);
    }
}

std::vector<std::shared_ptr<engine::core::plugin::ISubsystemInterface>> SubsystemRegistry::load(
    std::span<const std::string_view> requested) const
{
    std::unordered_set<std::string> requested_set{};
    requested_set.reserve(requested.size());
    for (const auto& name : requested)
    {
        requested_set.emplace(name);
    }

    std::unordered_set<std::string> enabled{};

    if (requested_set.empty())
    {
        for (const auto& descriptor : descriptors_)
        {
            if (descriptor.enabled_by_default)
            {
                gather_dependencies(descriptor.name, enabled);
            }
        }
    }
    else
    {
        for (const auto& name : requested_set)
        {
            gather_dependencies(name, enabled);
        }
    }

    std::vector<std::shared_ptr<engine::core::plugin::ISubsystemInterface>> plugins{};
    plugins.reserve(enabled.size());

    for (const auto& descriptor : descriptors_)
    {
        if (!enabled.contains(descriptor.name))
        {
            continue;
        }
        auto plugin = descriptor.factory();
        if (plugin != nullptr)
        {
            plugins.push_back(std::move(plugin));
        }
    }

    return plugins;
}

std::vector<std::shared_ptr<engine::core::plugin::ISubsystemInterface>> SubsystemRegistry::load_defaults() const
{
    constexpr std::string_view empty_selection[]{};
    return load(empty_selection);
}

SubsystemRegistry make_default_subsystem_registry()
{
    SubsystemRegistry registry{};
    (void)registry;
#if ENGINE_ENABLE_ANIMATION
    registry.register_subsystem(SubsystemDescriptor{
        std::string{engine::animation::module_name()},
        {},
        []() { return make_static_plugin(engine::animation::module_name()); },
        true});
#endif
#if ENGINE_ENABLE_ASSETS
    registry.register_subsystem(SubsystemDescriptor{
        std::string{engine::assets::module_name()},
        {},
        []() { return make_static_plugin(engine::assets::module_name()); },
        true});
#endif
#if ENGINE_ENABLE_COMPUTE
    registry.register_subsystem(SubsystemDescriptor{
        std::string{engine::compute::module_name()},
        {},
        []() { return make_static_plugin(engine::compute::module_name()); },
        true});
#endif
#if ENGINE_ENABLE_COMPUTE && ENGINE_ENABLE_COMPUTE_CUDA
    registry.register_subsystem(SubsystemDescriptor{
        std::string{engine::compute::cuda::module_name()},
        {std::string{engine::compute::module_name()}},
        []() {
            return make_static_plugin(
                engine::compute::cuda::module_name(),
                {engine::compute::module_name()});
        },
        true});
#endif
#if ENGINE_ENABLE_CORE
    registry.register_subsystem(SubsystemDescriptor{
        std::string{engine::core::module_name()},
        {},
        []() { return make_static_plugin(engine::core::module_name()); },
        true});
#endif
#if ENGINE_ENABLE_GEOMETRY
    registry.register_subsystem(SubsystemDescriptor{
        std::string{engine::geometry::module_name()},
        {},
        []() { return make_static_plugin(engine::geometry::module_name()); },
        true});
#endif
#if ENGINE_ENABLE_IO
    registry.register_subsystem(SubsystemDescriptor{
        std::string{engine::io::module_name()},
        {},
        []() { return make_static_plugin(engine::io::module_name()); },
        true});
#endif
#if ENGINE_ENABLE_PHYSICS
    registry.register_subsystem(SubsystemDescriptor{
        std::string{engine::physics::module_name()},
        {},
        []() { return make_static_plugin(engine::physics::module_name()); },
        true});
#endif
#if ENGINE_ENABLE_PLATFORM
    registry.register_subsystem(SubsystemDescriptor{
        std::string{engine::platform::module_name()},
        {},
        []() { return make_static_plugin(engine::platform::module_name()); },
        true});
#endif
#if ENGINE_ENABLE_RENDERING
    registry.register_subsystem(SubsystemDescriptor{
        std::string{engine::rendering::module_name()},
        {},
        []() { return make_static_plugin(engine::rendering::module_name()); },
        true});
#endif
#if ENGINE_ENABLE_SCENE
    registry.register_subsystem(SubsystemDescriptor{
        std::string{engine::scene::module_name()},
        {},
        []() { return make_static_plugin(engine::scene::module_name()); },
        true});
#endif
    return registry;
}

}  // namespace engine::runtime
