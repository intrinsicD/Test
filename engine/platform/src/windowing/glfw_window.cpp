#include "window_base.hpp"

#include <utility>

namespace engine::platform::windowing {

std::shared_ptr<Window> create_glfw_window(WindowConfig config,
                                           std::shared_ptr<EventQueue> queue) {
    return create_headless_window("glfw-stub", std::move(config), std::move(queue));
}

}  // namespace engine::platform::windowing

