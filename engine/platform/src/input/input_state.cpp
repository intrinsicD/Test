#include "engine/platform/input/input_state.hpp"

namespace engine::platform::input {

InputState::InputState() noexcept {
    reset();
}

void InputState::reset() noexcept {
    current_keys_.fill(false);
    previous_keys_.fill(false);

    current_mouse_buttons_.fill(false);
    previous_mouse_buttons_.fill(false);

    cursor_position_ = Vector2{};
    cursor_reference_ = Vector2{};
    cursor_delta_ = Vector2{};
    scroll_delta_ = Vector2{};
}

void InputState::begin_frame() noexcept {
    previous_keys_ = current_keys_;
    previous_mouse_buttons_ = current_mouse_buttons_;

    cursor_reference_ = cursor_position_;
    cursor_delta_ = Vector2{};
    scroll_delta_ = Vector2{};
}

void InputState::apply_key_event(Key key, bool pressed) noexcept {
    const auto index = key_index(key);
    if (index >= current_keys_.size()) {
        return;
    }
    current_keys_[index] = pressed;
}

void InputState::apply_mouse_button_event(MouseButton button, bool pressed) noexcept {
    const auto index = mouse_index(button);
    if (index >= current_mouse_buttons_.size()) {
        return;
    }
    current_mouse_buttons_[index] = pressed;
}

void InputState::apply_cursor_position(float x, float y) noexcept {
    cursor_position_ = Vector2{x, y};
    cursor_delta_.x = cursor_position_.x - cursor_reference_.x;
    cursor_delta_.y = cursor_position_.y - cursor_reference_.y;
}

void InputState::apply_scroll_delta(float x_offset, float y_offset) noexcept {
    scroll_delta_.x += x_offset;
    scroll_delta_.y += y_offset;
}

bool InputState::is_key_down(Key key) const noexcept {
    const auto index = key_index(key);
    if (index >= current_keys_.size()) {
        return false;
    }
    return current_keys_[index];
}

bool InputState::is_key_up(Key key) const noexcept {
    return !is_key_down(key);
}

bool InputState::was_key_pressed(Key key) const noexcept {
    const auto index = key_index(key);
    if (index >= current_keys_.size()) {
        return false;
    }
    return current_keys_[index] && !previous_keys_[index];
}

bool InputState::was_key_released(Key key) const noexcept {
    const auto index = key_index(key);
    if (index >= current_keys_.size()) {
        return false;
    }
    return !current_keys_[index] && previous_keys_[index];
}

bool InputState::is_mouse_button_down(MouseButton button) const noexcept {
    const auto index = mouse_index(button);
    if (index >= current_mouse_buttons_.size()) {
        return false;
    }
    return current_mouse_buttons_[index];
}

bool InputState::is_mouse_button_up(MouseButton button) const noexcept {
    return !is_mouse_button_down(button);
}

bool InputState::was_mouse_button_pressed(MouseButton button) const noexcept {
    const auto index = mouse_index(button);
    if (index >= current_mouse_buttons_.size()) {
        return false;
    }
    return current_mouse_buttons_[index] && !previous_mouse_buttons_[index];
}

bool InputState::was_mouse_button_released(MouseButton button) const noexcept {
    const auto index = mouse_index(button);
    if (index >= current_mouse_buttons_.size()) {
        return false;
    }
    return !current_mouse_buttons_[index] && previous_mouse_buttons_[index];
}

Vector2 InputState::cursor_position() const noexcept {
    return cursor_position_;
}

Vector2 InputState::cursor_delta() const noexcept {
    return cursor_delta_;
}

Vector2 InputState::scroll_delta() const noexcept {
    return scroll_delta_;
}

}  // namespace engine::platform::input

