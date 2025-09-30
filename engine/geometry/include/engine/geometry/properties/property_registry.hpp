#pragma once

#include <cassert>
#include <cstddef>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <typeindex>
#include <type_traits>
#include <utility>
#include <vector>

namespace engine::geometry {

using PropertyId = std::size_t;

class PropertyRegistry;

namespace detail {

class PropertyStorageBase {
public:
    explicit PropertyStorageBase(std::string name) : name_(std::move(name)) {}
    virtual ~PropertyStorageBase() = default;

    PropertyStorageBase(const PropertyStorageBase&) = delete;
    PropertyStorageBase& operator=(const PropertyStorageBase&) = delete;

    [[nodiscard]] const std::string& name() const noexcept { return name_; }

    [[nodiscard]] virtual std::unique_ptr<PropertyStorageBase> clone() const = 0;

    virtual void reserve(std::size_t n) = 0;
    virtual void resize(std::size_t n) = 0;
    virtual void shrink_to_fit() = 0;
    virtual void push_back() = 0;
    virtual void swap(std::size_t i0, std::size_t i1) = 0;

    [[nodiscard]] virtual std::type_index type() const noexcept = 0;

protected:
    std::string name_;
};

template <class T>
class PropertyStorage final : public PropertyStorageBase {
public:
    PropertyStorage(std::string name, T default_value)
        : PropertyStorageBase(std::move(name)), data_(), default_(std::move(default_value))
    {
    }

    PropertyStorage(const PropertyStorage& other)
        : PropertyStorageBase(other.name()), data_(other.data_), default_(other.default_)
    {
    }

    PropertyStorage& operator=(const PropertyStorage& other)
    {
        if (this != &other)
        {
            name_ = other.name_;
            data_ = other.data_;
            default_ = other.default_;
        }
        return *this;
    }

    [[nodiscard]] std::unique_ptr<PropertyStorageBase> clone() const override
    {
        return std::make_unique<PropertyStorage<T>>(*this);
    }

    void reserve(std::size_t n) override { data_.reserve(n); }
    void resize(std::size_t n) override { data_.resize(n, default_); }
    void shrink_to_fit() override { data_.shrink_to_fit(); }
    void push_back() override { data_.push_back(default_); }

    void swap(std::size_t i0, std::size_t i1) override
    {
        using std::swap;
        swap(data_[i0], data_[i1]);
    }

    [[nodiscard]] std::type_index type() const noexcept override { return typeid(T); }

    [[nodiscard]] std::vector<T>& data() noexcept { return data_; }
    [[nodiscard]] const std::vector<T>& data() const noexcept { return data_; }
    [[nodiscard]] const T& default_value() const noexcept { return default_; }

private:
    std::vector<T> data_;
    T default_;
};

} // namespace detail

template <class T>
class PropertyBuffer;

template <class T>
class ConstPropertyBuffer;

class PropertyRegistry {
public:
    PropertyRegistry() = default;
    ~PropertyRegistry() = default;

    PropertyRegistry(const PropertyRegistry& other);
    PropertyRegistry(PropertyRegistry&&) noexcept = default;
    PropertyRegistry& operator=(const PropertyRegistry& other);
    PropertyRegistry& operator=(PropertyRegistry&&) noexcept = default;

    [[nodiscard]] std::size_t size() const noexcept { return size_; }
    [[nodiscard]] std::size_t property_count() const noexcept { return storages_.size(); }

    [[nodiscard]] std::vector<std::string> property_names() const;

    void clear();
    void reserve(std::size_t n);
    void resize(std::size_t n);
    void shrink_to_fit();
    void push_back();
    void swap(std::size_t i0, std::size_t i1);

    [[nodiscard]] bool contains(std::string_view name) const;
    [[nodiscard]] std::optional<PropertyId> find(std::string_view name) const;

    template <class T>
    [[nodiscard]] std::optional<PropertyBuffer<T>> add(std::string name, T default_value = T());

    template <class T>
    [[nodiscard]] std::optional<PropertyBuffer<T>> get(std::string_view name);

    template <class T>
    [[nodiscard]] std::optional<ConstPropertyBuffer<T>> get(std::string_view name) const;

    template <class T>
    [[nodiscard]] std::optional<PropertyBuffer<T>> get(PropertyId id);

    template <class T>
    [[nodiscard]] std::optional<ConstPropertyBuffer<T>> get(PropertyId id) const;

    template <class T>
    [[nodiscard]] PropertyBuffer<T> get_or_add(std::string name, T default_value = T());

    template <class T>
    bool remove(PropertyBuffer<T>& handle);

    bool remove(PropertyId id);

private:
    [[nodiscard]] detail::PropertyStorageBase* storage(PropertyId id) noexcept;
    [[nodiscard]] const detail::PropertyStorageBase* storage(PropertyId id) const noexcept;

    template <class T>
    [[nodiscard]] detail::PropertyStorage<T>* storage(PropertyId id) noexcept;

    template <class T>
    [[nodiscard]] const detail::PropertyStorage<T>* storage(PropertyId id) const noexcept;

    std::vector<std::unique_ptr<detail::PropertyStorageBase>> storages_;
    std::size_t size_{0};
};

template <class T>
class PropertyBuffer {
public:
    PropertyBuffer() = default;

    [[nodiscard]] PropertyId id() const noexcept { return id_; }
    [[nodiscard]] const std::string& name() const noexcept
    {
        assert(storage_ != nullptr);
        return storage_->name();
    }
    [[nodiscard]] explicit operator bool() const noexcept { return storage_ != nullptr; }

    [[nodiscard]] std::vector<T>& vector() const noexcept
    {
        assert(storage_ != nullptr);
        return storage_->data();
    }

    [[nodiscard]] decltype(auto) operator[](std::size_t index) const
    {
        assert(storage_ != nullptr);
        assert(index < storage_->data().size());
        return storage_->data()[index];
    }

    [[nodiscard]] std::span<const T> span() const noexcept requires (!std::is_same_v<T, bool>)
    {
        assert(storage_ != nullptr);
        return std::span<T>(storage_->data());
    }

    [[nodiscard]] std::span<T> span() noexcept requires (!std::is_same_v<T, bool>)
    {
        assert(storage_ != nullptr);
        return std::span<T>(storage_->data());
    }

    [[nodiscard]] const T* data() const noexcept requires (!std::is_same_v<T, bool>)
    {
        assert(storage_ != nullptr);
        return storage_->data().data();
    }

    void reset() noexcept
    {
        storage_ = nullptr;
        id_ = static_cast<PropertyId>(-1);
    }

private:
    friend class PropertyRegistry;

    PropertyBuffer(PropertyId id, detail::PropertyStorage<T>* storage) : storage_(storage), id_(id) {}

    detail::PropertyStorage<T>* storage_{nullptr};
    PropertyId id_{static_cast<PropertyId>(-1)};
};

template <class T>
class ConstPropertyBuffer {
public:
    ConstPropertyBuffer() = default;

    [[nodiscard]] PropertyId id() const noexcept { return id_; }
    [[nodiscard]] const std::string& name() const noexcept
    {
        assert(storage_ != nullptr);
        return storage_->name();
    }
    [[nodiscard]] explicit operator bool() const noexcept { return storage_ != nullptr; }

    [[nodiscard]] const std::vector<T>& vector() const noexcept
    {
        assert(storage_ != nullptr);
        return storage_->data();
    }

    [[nodiscard]] decltype(auto) operator[](std::size_t index) const
    {
        assert(storage_ != nullptr);
        assert(index < storage_->data().size());
        return storage_->data()[index];
    }

    [[nodiscard]] std::span<const T> span() const noexcept requires (!std::is_same_v<T, bool>)
    {
        assert(storage_ != nullptr);
        return std::span<const T>(storage_->data());
    }

    [[nodiscard]] const T* data() const noexcept requires (!std::is_same_v<T, bool>)
    {
        assert(storage_ != nullptr);
        return storage_->data().data();
    }

private:
    friend class PropertyRegistry;

    ConstPropertyBuffer(PropertyId id, const detail::PropertyStorage<T>* storage)
        : storage_(storage), id_(id)
    {
    }

    const detail::PropertyStorage<T>* storage_{nullptr};
    PropertyId id_{static_cast<PropertyId>(-1)};
};

template <class T>
detail::PropertyStorage<T>* PropertyRegistry::storage(PropertyId id) noexcept
{
    auto* base = storage(id);
    if (base == nullptr || base->type() != typeid(T))
    {
        return nullptr;
    }
    return static_cast<detail::PropertyStorage<T>*>(base);
}

template <class T>
const detail::PropertyStorage<T>* PropertyRegistry::storage(PropertyId id) const noexcept
{
    auto* base = storage(id);
    if (base == nullptr || base->type() != typeid(T))
    {
        return nullptr;
    }
    return static_cast<const detail::PropertyStorage<T>*>(base);
}

template <class T>
std::optional<PropertyBuffer<T>> PropertyRegistry::add(std::string name, T default_value)
{
    if (find(name).has_value())
    {
        return std::nullopt;
    }

    auto storage = std::make_unique<detail::PropertyStorage<T>>(std::move(name), std::move(default_value));
    storage->resize(size_);
    auto* raw = storage.get();
    storages_.push_back(std::move(storage));
    return PropertyBuffer<T>(storages_.size() - 1U, raw);
}

template <class T>
std::optional<PropertyBuffer<T>> PropertyRegistry::get(std::string_view name)
{
    if (auto id = find(name))
    {
        return get<T>(*id);
    }
    return std::nullopt;
}

template <class T>
std::optional<ConstPropertyBuffer<T>> PropertyRegistry::get(std::string_view name) const
{
    if (auto id = find(name))
    {
        return get<T>(*id);
    }
    return std::nullopt;
}

template <class T>
std::optional<PropertyBuffer<T>> PropertyRegistry::get(PropertyId id)
{
    if (auto* typed = storage<T>(id))
    {
        return PropertyBuffer<T>(id, typed);
    }
    return std::nullopt;
}

template <class T>
std::optional<ConstPropertyBuffer<T>> PropertyRegistry::get(PropertyId id) const
{
    if (auto* typed = storage<T>(id))
    {
        return ConstPropertyBuffer<T>(id, typed);
    }
    return std::nullopt;
}

template <class T>
PropertyBuffer<T> PropertyRegistry::get_or_add(std::string name, T default_value)
{
    if (auto existing = get<T>(name))
    {
        return *existing;
    }

    auto created = add<T>(std::move(name), std::move(default_value));
    if (created)
    {
        return std::move(*created);
    }
    return PropertyBuffer<T>();
}

template <class T>
bool PropertyRegistry::remove(PropertyBuffer<T>& handle)
{
    if (!handle)
    {
        return false;
    }

    auto* typed = storage<T>(handle.id_);
    if (typed != handle.storage_)
    {
        return false;
    }

    const bool removed = remove(handle.id_);
    if (removed)
    {
        handle.reset();
    }
    return removed;
}

} // namespace engine::geometry

