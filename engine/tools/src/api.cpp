#include "engine/tools/api.hpp"

namespace engine::tools {

    std::string_view module_name() noexcept {
        return "tools";
    }

}  // namespace engine::tools

extern "C" const char* engine_tools_module_name() noexcept {
    return "tools";
}
