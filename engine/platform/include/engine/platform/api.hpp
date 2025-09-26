#pragma once

#include <string_view>

namespace engine::platform {

[[nodiscard]] std::string_view module_name() noexcept;

}  // namespace engine::platform
