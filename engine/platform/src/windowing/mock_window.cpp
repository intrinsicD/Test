#include "window_base.hpp"

#include <utility>

namespace engine::platform::windowing {

std::shared_ptr<Window> create_mock_window(WindowConfig config,
                                           std::shared_ptr<EventQueue> queue) {
    return create_headless_window("mock", std::move(config), std::move(queue));
}

}  // namespace engine::platform::windowing

