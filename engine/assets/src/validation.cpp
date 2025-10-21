#include "engine/assets/validation.hpp"

#include <algorithm>

namespace engine::assets
{
    HandleValidationTelemetry& HandleValidationTelemetry::instance()
    {
        static HandleValidationTelemetry telemetry{};
        return telemetry;
    }

    void HandleValidationTelemetry::record_success(std::string_view type, std::string_view identifier)
    {
        std::scoped_lock lock{mutex_};
        auto& counter = counters_[std::string{type}];
        counter.success += 1U;
        counter.last_identifier.assign(identifier);
        counter.last_context.clear();
        counter.last_reason.clear();
    }

    void HandleValidationTelemetry::record_failure(HandleValidationFailure failure)
    {
        std::scoped_lock lock{mutex_};
        auto& counter = counters_[failure.type];
        counter.failure += 1U;
        counter.last_identifier = std::move(failure.identifier);
        counter.last_context = std::move(failure.context);
        counter.last_reason = std::move(failure.reason);
    }

    std::vector<HandleValidationSnapshotEntry> HandleValidationTelemetry::snapshot() const
    {
        std::scoped_lock lock{mutex_};
        std::vector<HandleValidationSnapshotEntry> entries{};
        entries.reserve(counters_.size());
        for (const auto& [type, counter] : counters_)
        {
            HandleValidationSnapshotEntry entry{};
            entry.type = type;
            entry.success_count = counter.success;
            entry.failure_count = counter.failure;
            entry.last_failure_identifier = counter.last_identifier;
            entry.last_failure_context = counter.last_context;
            entry.last_failure_reason = counter.last_reason;
            entries.push_back(std::move(entry));
        }
        std::sort(entries.begin(), entries.end(), [](const auto& lhs, const auto& rhs) {
            return lhs.type < rhs.type;
        });
        return entries;
    }

    void HandleValidationTelemetry::reset()
    {
        std::scoped_lock lock{mutex_};
        counters_.clear();
    }

    HandleValidatorRegistry& HandleValidatorRegistry::instance()
    {
        static HandleValidatorRegistry registry{};
        return registry;
    }

    std::shared_ptr<void> HandleValidatorRegistry::register_mesh_validator(detail::ValidatorBucket<MeshHandle>::Validator validator)
    {
        return mesh_validators_.register_validator(std::move(validator));
    }

    std::shared_ptr<void> HandleValidatorRegistry::register_graph_validator(detail::ValidatorBucket<GraphHandle>::Validator validator)
    {
        return graph_validators_.register_validator(std::move(validator));
    }

    std::shared_ptr<void> HandleValidatorRegistry::register_point_cloud_validator(detail::ValidatorBucket<PointCloudHandle>::Validator validator)
    {
        return point_cloud_validators_.register_validator(std::move(validator));
    }

    std::shared_ptr<void> HandleValidatorRegistry::register_texture_validator(detail::ValidatorBucket<TextureHandle>::Validator validator)
    {
        return texture_validators_.register_validator(std::move(validator));
    }

    std::shared_ptr<void> HandleValidatorRegistry::register_shader_validator(detail::ValidatorBucket<ShaderHandle>::Validator validator)
    {
        return shader_validators_.register_validator(std::move(validator));
    }

    std::shared_ptr<void> HandleValidatorRegistry::register_material_validator(detail::ValidatorBucket<MaterialHandle>::Validator validator)
    {
        return material_validators_.register_validator(std::move(validator));
    }

    bool HandleValidatorRegistry::validate(const MeshHandle& handle) const
    {
        return mesh_validators_.validate(handle);
    }

    bool HandleValidatorRegistry::validate(const GraphHandle& handle) const
    {
        return graph_validators_.validate(handle);
    }

    bool HandleValidatorRegistry::validate(const PointCloudHandle& handle) const
    {
        return point_cloud_validators_.validate(handle);
    }

    bool HandleValidatorRegistry::validate(const TextureHandle& handle) const
    {
        return texture_validators_.validate(handle);
    }

    bool HandleValidatorRegistry::validate(const ShaderHandle& handle) const
    {
        return shader_validators_.validate(handle);
    }

    bool HandleValidatorRegistry::validate(const MaterialHandle& handle) const
    {
        return material_validators_.validate(handle);
    }
}  // namespace engine::assets

