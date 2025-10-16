#pragma once

#include <cassert>
#include <cstdint>
#include <limits>
#include <optional>
#include <stdexcept>
#include <utility>
#include <vector>

namespace engine::core::memory {

/// Handle that identifies a resource slot inside a ResourcePool.
///
/// Each handle stores the slot index alongside a generation counter. When a
/// slot is released the generation counter increments, invalidating previously
/// issued handles so stale references are rejected the next time they are used.
template <typename Tag = void>
struct GenerationalHandle {
    static constexpr std::uint32_t invalid_index =
        std::numeric_limits<std::uint32_t>::max();

    std::uint32_t index{invalid_index};
    std::uint32_t generation{0};

    [[nodiscard]] bool is_valid() const noexcept { return generation != 0U; }

    friend bool operator==(const GenerationalHandle& lhs,
                           const GenerationalHandle& rhs) noexcept = default;
};

template <typename Tag = void>
struct GenerationalHandleHasher {
    [[nodiscard]] std::size_t operator()(const GenerationalHandle<Tag>& handle) const noexcept
    {
        return (static_cast<std::size_t>(handle.index) << 32U) ^
               static_cast<std::size_t>(handle.generation);
    }
};

/// Pool that manages a dense set of resources referenced through generational
/// handles. Slots are recycled without invalidating live handles, ensuring that
/// consumers can detect stale references reliably.
template <typename T, typename HandleTag = void>
class ResourcePool {
public:
    using handle_type = GenerationalHandle<HandleTag>;
    using handle_hasher = GenerationalHandleHasher<HandleTag>;

    ResourcePool() = default;

    ResourcePool(const ResourcePool&) = delete;
    ResourcePool& operator=(const ResourcePool&) = delete;

    ResourcePool(ResourcePool&&) noexcept = default;
    ResourcePool& operator=(ResourcePool&&) noexcept = default;

    /// Acquire a slot and construct a resource in place, returning the handle
    /// and a reference to the stored value.
    template <typename... Args>
    [[nodiscard]] std::pair<handle_type, T&> acquire(Args&&... args)
    {
        const std::uint32_t index = allocate_slot();
        Slot& slot = slots_[index];
        if (slot.generation == 0U) {
            slot.generation = 1U;
        }

        slot.value.emplace(std::forward<Args>(args)...);
        ++active_count_;

        handle_type handle;
        handle.index = index;
        handle.generation = slot.generation;
        return {handle, *slot.value};
    }

    /// Check whether the provided handle references a live resource.
    [[nodiscard]] bool is_valid(handle_type handle) const noexcept
    {
        return handle.index < slots_.size() &&
               slots_[handle.index].value.has_value() &&
               slots_[handle.index].generation == handle.generation;
    }

    /// Obtain a mutable reference to the resource identified by the handle.
    ///
    /// Throws std::out_of_range when the handle is stale or invalid.
    [[nodiscard]] T& get(handle_type handle)
    {
        assert(is_valid(handle));
        if (!is_valid(handle)) {
            throw std::out_of_range("ResourcePool handle is not valid");
        }
        return *slots_[handle.index].value;
    }

    /// Obtain an immutable reference to the resource identified by the handle.
    [[nodiscard]] const T& get(handle_type handle) const
    {
        assert(is_valid(handle));
        if (!is_valid(handle)) {
            throw std::out_of_range("ResourcePool handle is not valid");
        }
        return *slots_[handle.index].value;
    }

    /// Release the resource referenced by the handle. Stale handles are
    /// ignored to simplify teardown paths.
    void release(handle_type handle) noexcept
    {
        if (!is_valid(handle)) {
            return;
        }

        Slot& slot = slots_[handle.index];
        slot.value.reset();
        ++slot.generation;
        free_list_.push_back(handle.index);

        if (active_count_ > 0U) {
            --active_count_;
        }
    }

    /// Release every live resource and recycle all slots.
    void clear() noexcept
    {
        for (auto& slot : slots_) {
            slot.value.reset();
            if (slot.generation != 0U) {
                ++slot.generation;
            }
        }
        free_list_.clear();
        active_count_ = 0U;
    }

    /// Visit each live resource, providing the associated handle.
    template <typename Visitor>
    void for_each(Visitor&& visitor)
    {
        for (std::uint32_t index = 0U; index < slots_.size(); ++index) {
            Slot& slot = slots_[index];
            if (!slot.value.has_value()) {
                continue;
            }

            handle_type handle;
            handle.index = index;
            handle.generation = slot.generation;
            visitor(handle, *slot.value);
        }
    }

    /// Const-qualified overload of for_each.
    template <typename Visitor>
    void for_each(Visitor&& visitor) const
    {
        for (std::uint32_t index = 0U; index < slots_.size(); ++index) {
            const Slot& slot = slots_[index];
            if (!slot.value.has_value()) {
                continue;
            }

            handle_type handle;
            handle.index = index;
            handle.generation = slot.generation;
            visitor(handle, *slot.value);
        }
    }

    [[nodiscard]] std::size_t active_count() const noexcept { return active_count_; }

    [[nodiscard]] bool empty() const noexcept { return active_count_ == 0U; }

private:
    struct Slot {
        std::optional<T> value{};
        std::uint32_t generation{0U};
    };

    [[nodiscard]] std::uint32_t allocate_slot()
    {
        if (!free_list_.empty()) {
            const std::uint32_t index = free_list_.back();
            free_list_.pop_back();
            return index;
        }

        const std::uint32_t index = static_cast<std::uint32_t>(slots_.size());
        slots_.emplace_back();
        return index;
    }

    std::vector<Slot> slots_{};
    std::vector<std::uint32_t> free_list_{};
    std::size_t active_count_{0U};
};

}  // namespace engine::core::memory

