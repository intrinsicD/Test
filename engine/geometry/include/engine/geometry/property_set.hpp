#pragma once

#include "engine/geometry/property_registry.hpp"

#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace engine::geometry {

template <class T>
class Property {
public:
    Property() = default;
    explicit Property(PropertyBuffer<T> buffer) : buffer_(buffer) {}

    [[nodiscard]] bool is_valid() const noexcept { return static_cast<bool>(buffer_); }

    [[nodiscard]] decltype(auto) operator[](std::size_t index) const { return buffer_[index]; }
    [[nodiscard]] decltype(auto) operator[](std::size_t index) { return buffer_[index]; }

    [[nodiscard]] std::vector<T>& vector() { return buffer_.vector(); }
    [[nodiscard]] const std::vector<T>& vector() const { return buffer_.vector(); }

    [[nodiscard]] std::vector<T>& array() { return buffer_.vector(); }
    [[nodiscard]] const std::vector<T>& array() const { return buffer_.vector(); }

    [[nodiscard]] PropertyBuffer<T>& handle() noexcept { return buffer_; }
    [[nodiscard]] const PropertyBuffer<T>& handle() const noexcept { return buffer_; }

    void reset() noexcept { buffer_.reset(); }

private:
    PropertyBuffer<T> buffer_;
};

template <class HandleT, class T>
class HandleProperty : public Property<T> {
public:
    using Property<T>::Property;

    explicit HandleProperty(Property<T> base) : Property<T>(std::move(base)) {}

    [[nodiscard]] decltype(auto) operator[](HandleT handle) { return Property<T>::operator[](handle.index()); }
    [[nodiscard]] decltype(auto) operator[](HandleT handle) const { return Property<T>::operator[](handle.index()); }
};

class MeshPropertySet {
public:
    MeshPropertySet() = default;

    [[nodiscard]] std::size_t size() const noexcept { return registry_.size(); }

    void clear();
    void reserve(std::size_t n);
    void resize(std::size_t n);
    void push_back();
    void swap(std::size_t i0, std::size_t i1);
    void shrink_to_fit();

    [[nodiscard]] bool exists(std::string_view name) const;
    [[nodiscard]] std::vector<std::string> properties() const;

    template <class T>
    [[nodiscard]] Property<T> add(std::string name, T default_value = T());

    template <class T>
    [[nodiscard]] Property<T> get(std::string_view name);

    template <class T>
    [[nodiscard]] Property<T> get(std::string_view name) const;

    template <class T>
    [[nodiscard]] Property<T> get_or_add(std::string name, T default_value = T());

    template <class T>
    void remove(Property<T>& property);

    PropertyRegistry& registry() noexcept { return registry_; }
    const PropertyRegistry& registry() const noexcept { return registry_; }

private:
    PropertyRegistry registry_;
};

inline void MeshPropertySet::clear()
{
    registry_.clear();
}

inline void MeshPropertySet::reserve(std::size_t n)
{
    registry_.reserve(n);
}

inline void MeshPropertySet::resize(std::size_t n)
{
    registry_.resize(n);
}

inline void MeshPropertySet::push_back()
{
    registry_.push_back();
}

inline void MeshPropertySet::swap(std::size_t i0, std::size_t i1)
{
    registry_.swap(i0, i1);
}

inline void MeshPropertySet::shrink_to_fit()
{
    registry_.shrink_to_fit();
}

inline bool MeshPropertySet::exists(std::string_view name) const
{
    return registry_.contains(name);
}

inline std::vector<std::string> MeshPropertySet::properties() const
{
    return registry_.property_names();
}

template <class T>
inline Property<T> MeshPropertySet::add(std::string name, T default_value)
{
    if (auto handle = registry_.add<T>(std::move(name), std::move(default_value)))
    {
        return Property<T>(*handle);
    }
    return Property<T>();
}

template <class T>
inline Property<T> MeshPropertySet::get(std::string_view name)
{
    if (auto handle = registry_.get<T>(name))
    {
        return Property<T>(*handle);
    }
    return Property<T>();
}

template <class T>
inline Property<T> MeshPropertySet::get(std::string_view name) const
{
    return const_cast<MeshPropertySet*>(this)->get<T>(name);
}

template <class T>
inline Property<T> MeshPropertySet::get_or_add(std::string name, T default_value)
{
    auto handle = registry_.get_or_add<T>(std::move(name), std::move(default_value));
    return Property<T>(handle);
}

template <class T>
inline void MeshPropertySet::remove(Property<T>& property)
{
    registry_.remove(property.handle());
    property.reset();
}

} // namespace engine::geometry

