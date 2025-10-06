#pragma once

#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "engine/core/api.hpp"
#include "engine/core/ecs/registry.hpp"

namespace engine::core::ecs {

class system {
public:
    virtual ~system() = default;
    [[nodiscard]] virtual std::string_view name() const noexcept = 0;
    virtual void update(registry& registry, double dt) = 0;
};

class lambda_system final : public system {
public:
    using callback_type = std::function<void(registry&, double)>;

    lambda_system(std::string name, callback_type callback);

    [[nodiscard]] std::string_view name() const noexcept override;
    void update(registry& registry, double dt) override;

private:
    std::string name_;
    callback_type callback_;
};

class system_scheduler {
public:
    using system_ptr = std::unique_ptr<system>;

    void add_system(system_ptr system);

    template <typename Func>
    void add_lambda_system(std::string name, Func&& func) {
        add_system(std::make_unique<lambda_system>(std::move(name), std::forward<Func>(func)));
    }

    void tick(registry& registry, double dt);

    [[nodiscard]] std::size_t system_count() const noexcept {
        return systems_.size();
    }

private:
    std::vector<system_ptr> systems_;
};

}  // namespace engine::core::ecs

