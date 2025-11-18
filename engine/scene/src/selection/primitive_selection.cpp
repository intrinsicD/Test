#include "engine/scene/selection/primitive_selection.hpp"

#include <algorithm>

namespace engine::scene::selection
{
    namespace
    {
        [[nodiscard]] std::size_t clamp_positive(std::size_t value, std::size_t fallback) noexcept
        {
            return value == 0 ? fallback : value;
        }
    } // namespace

    bool PrimitiveHit::is_edge() const noexcept
    {
        return primitive == SelectionPrimitive::Edge;
    }

    bool PrimitiveHit::is_face() const noexcept
    {
        return primitive == SelectionPrimitive::Face;
    }

    PrimitiveHitBuffer::ChunkRange::Iterator::Iterator(
        const PrimitiveHitBuffer* owner,
        std::size_t index,
        std::size_t chunk_size
    ) noexcept : owner_(owner), index_(index), chunk_size_(chunk_size)
    {
    }

    PrimitiveHitBuffer::ChunkRange::Iterator::value_type PrimitiveHitBuffer::ChunkRange::Iterator::operator*() const noexcept
    {
        if (!owner_ || owner_->hits_.empty() || index_ >= owner_->hits_.size())
        {
            return {};
        }

        const auto chunk_end = std::min(owner_->hits_.size(), index_ + chunk_size_);
        return std::span<const PrimitiveHit>{owner_->hits_.data() + static_cast<std::ptrdiff_t>(index_),
                                             chunk_end - index_};
    }

    PrimitiveHitBuffer::ChunkRange::Iterator& PrimitiveHitBuffer::ChunkRange::Iterator::operator++() noexcept
    {
        if (owner_ && index_ < owner_->hits_.size())
        {
            index_ = std::min(owner_->hits_.size(), index_ + chunk_size_);
        }
        return *this;
    }

    bool PrimitiveHitBuffer::ChunkRange::Iterator::operator==(const Iterator& other) const noexcept
    {
        return owner_ == other.owner_ && index_ == other.index_ && chunk_size_ == other.chunk_size_;
    }

    PrimitiveHitBuffer::ChunkRange::ChunkRange(const PrimitiveHitBuffer* owner, std::size_t chunk_size) noexcept
        : owner_(owner), chunk_size_(chunk_size)
    {
    }

    PrimitiveHitBuffer::ChunkRange::Iterator PrimitiveHitBuffer::ChunkRange::begin() const noexcept
    {
        return Iterator{owner_, 0, chunk_size_};
    }

    PrimitiveHitBuffer::ChunkRange::Iterator PrimitiveHitBuffer::ChunkRange::end() const noexcept
    {
        const auto size = owner_ ? owner_->hits_.size() : 0U;
        return Iterator{owner_, size, chunk_size_};
    }

    void PrimitiveHitBuffer::clear() noexcept
    {
        hits_.clear();
    }

    bool PrimitiveHitBuffer::empty() const noexcept
    {
        return hits_.empty();
    }

    bool PrimitiveHitBuffer::full() const noexcept
    {
        return hits_.size() >= max_primitives_;
    }

    void PrimitiveHitBuffer::set_chunk_size(std::size_t chunk_size) noexcept
    {
        chunk_size_ = clamp_positive(chunk_size, 1);
    }

    void PrimitiveHitBuffer::set_max_primitives(std::size_t max_primitives) noexcept
    {
        max_primitives_ = clamp_positive(max_primitives, 1);
        if (hits_.size() > max_primitives_)
        {
            hits_.resize(max_primitives_);
        }
    }

    bool PrimitiveHitBuffer::append(const PrimitiveHit& hit)
    {
        if (full())
        {
            return false;
        }

        hits_.push_back(hit);
        return true;
    }

    std::size_t PrimitiveHitBuffer::size() const noexcept
    {
        return hits_.size();
    }

    std::span<const PrimitiveHit> PrimitiveHitBuffer::hits() const noexcept
    {
        return hits_;
    }

    PrimitiveHitBuffer::ChunkRange PrimitiveHitBuffer::chunks() const noexcept
    {
        return ChunkRange{this, chunk_size_};
    }

    PrimitiveSelectionRegistry::AdapterId PrimitiveSelectionRegistry::register_adapter(
        std::unique_ptr<PrimitiveSelectionAdapter> adapter
    )
    {
        if (!adapter)
        {
            return 0;
        }

        const AdapterId id = next_id_++;
        adapters_.push_back(AdapterEntry{.id = id, .adapter = std::move(adapter)});
        return id;
    }

    void PrimitiveSelectionRegistry::unregister_adapter(AdapterId id)
    {
        adapters_.erase(
            std::remove_if(adapters_.begin(), adapters_.end(), [id](const AdapterEntry& entry) { return entry.id == id; }),
            adapters_.end()
        );
    }

    void PrimitiveSelectionRegistry::clear() noexcept
    {
        adapters_.clear();
        next_id_ = 1;
    }

    PrimitiveHitBuffer PrimitiveSelectionRegistry::gather_primitives(const PrimitivePickRequest& request) const
    {
        PrimitiveHitBuffer buffer;
        buffer.set_max_primitives(request.max_results);

        for (const auto& entry : adapters_)
        {
            if (!entry.adapter || buffer.full())
            {
                break;
            }

            if (!entry.adapter->supports(request))
            {
                continue;
            }

            entry.adapter->gather_primitives(request, buffer);
        }

        return buffer;
    }

    PrimitiveHitBuffer PrimitiveSelectionRegistry::marquee_select(const MarqueeRequest& request) const
    {
        PrimitiveHitBuffer buffer;
        buffer.set_max_primitives(request.max_results);

        for (const auto& entry : adapters_)
        {
            if (!entry.adapter || buffer.full())
            {
                break;
            }

            entry.adapter->marquee_select(request, buffer);
        }

        return buffer;
    }
} // namespace engine::scene::selection
