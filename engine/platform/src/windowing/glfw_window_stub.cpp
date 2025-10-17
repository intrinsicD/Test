#include "window_base.hpp"

#include <memory>
#include <stdexcept>

namespace engine::platform::windowing {

std::shared_ptr<Window> create_glfw_window(WindowConfig config,
                                           std::shared_ptr<EventQueue> queue) {
    (void)config;
    (void)queue;
    throw std::runtime_error(
        "GLFW backend is unavailable. Install the required dependencies and "
        "reconfigure with ENGINE_ENABLE_GLFW=ON to enable it.");
}

}  // namespace engine::platform::windowing
