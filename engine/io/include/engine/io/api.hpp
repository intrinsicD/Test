#pragma once

#include <string_view>

namespace engine::io {

[[nodiscard]] std::string_view module_name() noexcept;

}  // namespace engine::io
