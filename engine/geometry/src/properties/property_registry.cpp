#include "../../include/engine/geometry/properties/property_registry.hpp"

#include <algorithm>
#include <cassert>

namespace engine::geometry {

PropertyRegistry::PropertyRegistry(const PropertyRegistry& other)
    : storages_(), size_(other.size_)
{
    storages_.reserve(other.storages_.size());
    for (const auto& storage : other.storages_)
    {
        storages_.push_back(storage->clone());
    }
}

PropertyRegistry& PropertyRegistry::operator=(const PropertyRegistry& other)
{
    if (this == &other)
    {
        return *this;
    }

    storages_.clear();
    storages_.reserve(other.storages_.size());
    for (const auto& storage : other.storages_)
    {
        storages_.push_back(storage->clone());
    }
    size_ = other.size_;
    return *this;
}

std::vector<std::string> PropertyRegistry::property_names() const
{
    std::vector<std::string> names;
    names.reserve(storages_.size());
    for (const auto& storage : storages_)
    {
        names.emplace_back(storage->name());
    }
    return names;
}

void PropertyRegistry::clear()
{
    storages_.clear();
    size_ = 0;
}

void PropertyRegistry::reserve(std::size_t n)
{
    for (auto& storage : storages_)
    {
        storage->reserve(n);
    }
}

void PropertyRegistry::resize(std::size_t n)
{
    for (auto& storage : storages_)
    {
        storage->resize(n);
    }
    size_ = n;
}

void PropertyRegistry::shrink_to_fit()
{
    for (auto& storage : storages_)
    {
        storage->shrink_to_fit();
    }
}

void PropertyRegistry::push_back()
{
    for (auto& storage : storages_)
    {
        storage->push_back();
    }
    ++size_;
}

void PropertyRegistry::swap(std::size_t i0, std::size_t i1)
{
    assert(i0 < size_ && i1 < size_);
    if (i0 == i1)
    {
        return;
    }

    for (auto& storage : storages_)
    {
        storage->swap(i0, i1);
    }
}

bool PropertyRegistry::contains(std::string_view name) const
{
    return find(name).has_value();
}

std::optional<PropertyId> PropertyRegistry::find(std::string_view name) const
{
    for (PropertyId id = 0; id < storages_.size(); ++id)
    {
        if (storages_[id]->name() == name)
        {
            return id;
        }
    }
    return std::nullopt;
}

bool PropertyRegistry::remove(PropertyId id)
{
    if (id >= storages_.size())
    {
        return false;
    }

    storages_.erase(storages_.begin() + static_cast<std::ptrdiff_t>(id));
    return true;
}

detail::PropertyStorageBase* PropertyRegistry::storage(PropertyId id) noexcept
{
    if (id >= storages_.size())
    {
        return nullptr;
    }
    return storages_[id].get();
}

const detail::PropertyStorageBase* PropertyRegistry::storage(PropertyId id) const noexcept
{
    if (id >= storages_.size())
    {
        return nullptr;
    }
    return storages_[id].get();
}

} // namespace engine::geometry

