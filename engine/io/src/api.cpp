#include "engine/io/api.hpp"

namespace engine::io {

std::string_view module_name() noexcept {
    return "io";
}

}  // namespace engine::io

extern "C" ENGINE_IO_API const char* engine_io_module_name() noexcept {
    return engine::io::module_name().data();
}
