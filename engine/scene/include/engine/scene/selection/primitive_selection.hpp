#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <vector>
#include <iterator>

#include <entt/entt.hpp>

#include "engine/geometry/shapes/frustum.hpp"
#include "engine/math/vector.hpp"
#include "engine/scene/selection/selection_engine.hpp"

namespace engine::scene::selection
{
    enum class SelectionPrimitive
    {
        Vertex,
        Edge,
        Face,
        Point,
    };

    struct PrimitiveHit
    {
        entt::entity entity{entt::null};
        SelectionPrimitive primitive{SelectionPrimitive::Vertex};
        std::uint32_t index0{0};
        std::uint32_t index1{0};
        math::vec3 barycentric{0.0F, 0.0F, 0.0F};

        [[nodiscard]] bool is_edge() const noexcept;
        [[nodiscard]] bool is_face() const noexcept;
    };

    class PrimitiveHitBuffer
    {
    public:
        class ChunkRange
        {
        public:
            class Iterator
            {
            public:
                using iterator_category = std::input_iterator_tag;
                using value_type = std::span<const PrimitiveHit>;
                using difference_type = std::ptrdiff_t;

                Iterator(const PrimitiveHitBuffer* owner, std::size_t index, std::size_t chunk_size) noexcept;

                [[nodiscard]] value_type operator*() const noexcept;
                Iterator& operator++() noexcept;
                bool operator==(const Iterator& other) const noexcept;

            private:
                const PrimitiveHitBuffer* owner_{nullptr};
                std::size_t index_{0};
                std::size_t chunk_size_{0};
            };

            ChunkRange(const PrimitiveHitBuffer* owner, std::size_t chunk_size) noexcept;

            [[nodiscard]] Iterator begin() const noexcept;
            [[nodiscard]] Iterator end() const noexcept;

        private:
            const PrimitiveHitBuffer* owner_{nullptr};
            std::size_t chunk_size_{0};
        };

        void clear() noexcept;
        [[nodiscard]] bool empty() const noexcept;
        [[nodiscard]] bool full() const noexcept;
        void set_chunk_size(std::size_t chunk_size) noexcept;
        void set_max_primitives(std::size_t max_primitives) noexcept;
        bool append(const PrimitiveHit& hit);
        [[nodiscard]] std::size_t size() const noexcept;
        [[nodiscard]] std::span<const PrimitiveHit> hits() const noexcept;
        [[nodiscard]] ChunkRange chunks() const noexcept;

    private:
        std::vector<PrimitiveHit> hits_{};
        std::size_t chunk_size_{64};
        std::size_t max_primitives_{512};
    };

    struct PrimitivePickRequest
    {
        SelectionHit hit{};
        SelectionContext context{};
        std::size_t max_results{64};
    };

    struct MarqueeRequest
    {
        geometry::Frustum region{};
        SelectionContext context{};
        std::size_t max_results{256};
    };

    class PrimitiveSelectionAdapter
    {
    public:
        virtual ~PrimitiveSelectionAdapter() = default;
        [[nodiscard]] virtual bool supports(const PrimitivePickRequest& request) const noexcept = 0;
        virtual void gather_primitives(
            const PrimitivePickRequest& request,
            PrimitiveHitBuffer& out
        ) const = 0;
        virtual void marquee_select(
            const MarqueeRequest& request,
            PrimitiveHitBuffer& out
        ) const = 0;
    };

    class PrimitiveSelectionRegistry
    {
    public:
        using AdapterId = std::size_t;

        AdapterId register_adapter(std::unique_ptr<PrimitiveSelectionAdapter> adapter);
        void unregister_adapter(AdapterId id);
        void clear() noexcept;

        [[nodiscard]] PrimitiveHitBuffer gather_primitives(const PrimitivePickRequest& request) const;
        [[nodiscard]] PrimitiveHitBuffer marquee_select(const MarqueeRequest& request) const;

    private:
        struct AdapterEntry
        {
            AdapterId id{0};
            std::unique_ptr<PrimitiveSelectionAdapter> adapter{};
        };

        std::vector<AdapterEntry> adapters_{};
        AdapterId next_id_{1};
    };
} // namespace engine::scene::selection
