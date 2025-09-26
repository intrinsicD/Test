#pragma once

#include <string_view>

namespace engine::compute::cuda {

[[nodiscard]] std::string_view module_name() noexcept;

}  // namespace engine::compute::cuda
