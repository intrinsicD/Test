#pragma once

#include "engine/assets/handles.hpp"

#include <cassert>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

namespace engine::assets
{
    struct HandleValidationFailure
    {
        std::string type{};
        std::string identifier{};
        std::string context{};
        std::string reason{};
    };

    struct HandleValidationResult
    {
        bool valid{true};
        std::optional<HandleValidationFailure> failure{};
    };

    struct HandleValidationSnapshotEntry
    {
        std::string type{};
        std::uint64_t success_count{0};
        std::uint64_t failure_count{0};
        std::string last_failure_identifier{};
        std::string last_failure_context{};
        std::string last_failure_reason{};
    };

    class HandleValidationTelemetry
    {
    public:
        static HandleValidationTelemetry& instance();

        void record_success(std::string_view type, std::string_view identifier);
        void record_failure(HandleValidationFailure failure);
        [[nodiscard]] std::vector<HandleValidationSnapshotEntry> snapshot() const;
        void reset();

    private:
        struct Counter
        {
            std::uint64_t success{0};
            std::uint64_t failure{0};
            std::string last_identifier{};
            std::string last_context{};
            std::string last_reason{};
        };

        mutable std::mutex mutex_{};
        std::unordered_map<std::string, Counter> counters_{};
    };

    namespace detail
    {
        template <typename Handle>
        class ValidatorBucket
        {
        public:
            using Validator = std::function<bool(const Handle&)>;

            [[nodiscard]] std::shared_ptr<void> register_validator(Validator validator)
            {
                auto entry = std::make_shared<Validator>(std::move(validator));
                std::scoped_lock lock{mutex_};
                validators_.push_back(entry);
                return entry;
            }

            [[nodiscard]] bool validate(const Handle& handle) const
            {
                std::scoped_lock lock{mutex_};
                bool has_validator = false;
                auto it = validators_.begin();
                while (it != validators_.end())
                {
                    if (auto validator = it->lock())
                    {
                        has_validator = true;
                        if ((*validator)(handle))
                        {
                            return true;
                        }
                        ++it;
                        continue;
                    }
                    it = validators_.erase(it);
                }

                return !has_validator;
            }

        private:
            mutable std::mutex mutex_{};
            mutable std::vector<std::weak_ptr<Validator>> validators_{};
        };

        template <typename Handle>
        [[nodiscard]] constexpr std::string_view handle_type_name() noexcept;

        template <>
        [[nodiscard]] constexpr std::string_view handle_type_name<MeshHandle>() noexcept
        {
            return "MeshHandle";
        }

        template <>
        [[nodiscard]] constexpr std::string_view handle_type_name<GraphHandle>() noexcept
        {
            return "GraphHandle";
        }

        template <>
        [[nodiscard]] constexpr std::string_view handle_type_name<PointCloudHandle>() noexcept
        {
            return "PointCloudHandle";
        }

        template <>
        [[nodiscard]] constexpr std::string_view handle_type_name<TextureHandle>() noexcept
        {
            return "TextureHandle";
        }

        template <>
        [[nodiscard]] constexpr std::string_view handle_type_name<ShaderHandle>() noexcept
        {
            return "ShaderHandle";
        }

        template <>
        [[nodiscard]] constexpr std::string_view handle_type_name<MaterialHandle>() noexcept
        {
            return "MaterialHandle";
        }
    } // namespace detail

    class HandleValidatorRegistry
    {
    public:
        static HandleValidatorRegistry& instance();

        [[nodiscard]] std::shared_ptr<void> register_mesh_validator(detail::ValidatorBucket<MeshHandle>::Validator validator);
        [[nodiscard]] std::shared_ptr<void> register_graph_validator(detail::ValidatorBucket<GraphHandle>::Validator validator);
        [[nodiscard]] std::shared_ptr<void> register_point_cloud_validator(detail::ValidatorBucket<PointCloudHandle>::Validator validator);
        [[nodiscard]] std::shared_ptr<void> register_texture_validator(detail::ValidatorBucket<TextureHandle>::Validator validator);
        [[nodiscard]] std::shared_ptr<void> register_shader_validator(detail::ValidatorBucket<ShaderHandle>::Validator validator);
        [[nodiscard]] std::shared_ptr<void> register_material_validator(detail::ValidatorBucket<MaterialHandle>::Validator validator);

        [[nodiscard]] bool validate(const MeshHandle& handle) const;
        [[nodiscard]] bool validate(const GraphHandle& handle) const;
        [[nodiscard]] bool validate(const PointCloudHandle& handle) const;
        [[nodiscard]] bool validate(const TextureHandle& handle) const;
        [[nodiscard]] bool validate(const ShaderHandle& handle) const;
        [[nodiscard]] bool validate(const MaterialHandle& handle) const;

    private:
        detail::ValidatorBucket<MeshHandle> mesh_validators_{};
        detail::ValidatorBucket<GraphHandle> graph_validators_{};
        detail::ValidatorBucket<PointCloudHandle> point_cloud_validators_{};
        detail::ValidatorBucket<TextureHandle> texture_validators_{};
        detail::ValidatorBucket<ShaderHandle> shader_validators_{};
        detail::ValidatorBucket<MaterialHandle> material_validators_{};
    };

    template <typename Handle>
    [[nodiscard]] HandleValidationResult validate_handle_status(const Handle& handle, std::string_view context)
    {
        using detail::handle_type_name;

        HandleValidationResult result{};
        result.valid = true;

        if (handle.empty())
        {
            return result;
        }

        const auto type = std::string{handle_type_name<Handle>()};
        const auto identifier = std::string{handle.id()};

        if (!handle.is_bound())
        {
            result.valid = false;
            result.failure = HandleValidationFailure{type, identifier, std::string{context}, "Handle is not bound"};
            HandleValidationTelemetry::instance().record_failure(*result.failure);
            return result;
        }

        const bool valid = HandleValidatorRegistry::instance().validate(handle);
        if (!valid)
        {
            result.valid = false;
            result.failure = HandleValidationFailure{type, identifier, std::string{context}, "Handle validator rejected handle"};
            HandleValidationTelemetry::instance().record_failure(*result.failure);
            return result;
        }

        HandleValidationTelemetry::instance().record_success(type, identifier);
        return result;
    }

    template <typename Handle>
    [[nodiscard]] bool validate_handle(const Handle& handle, std::string_view context)
    {
        const auto result = validate_handle_status(handle, context);
#ifndef NDEBUG
        if (!result.valid)
        {
            assert((result.valid) && "Resource handle validation failed");
        }
#endif
        return result.valid;
    }
}  // namespace engine::assets

