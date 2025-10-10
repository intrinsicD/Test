#pragma once

#include "engine/platform/api.hpp"

#include <array>
#include <cstddef>
#include <cstdint>

namespace engine::platform::input {

/// \brief Enumerates the keyboard keys tracked by the mock input subsystem.
enum class Key : std::uint16_t {
    Unknown = 0,
    Escape,
    Space,
    Enter,
    Tab,
    Backspace,
    LeftShift,
    RightShift,
    LeftCtrl,
    RightCtrl,
    LeftAlt,
    RightAlt,
    LeftSuper,
    RightSuper,
    Up,
    Down,
    Left,
    Right,
    W,
    A,
    S,
    D,
    Q,
    E,
    Digit0,
    Digit1,
    Digit2,
    Digit3,
    Digit4,
    Digit5,
    Digit6,
    Digit7,
    Digit8,
    Digit9,
    Count,
};

/// \brief Enumerates mouse buttons recognised by the input subsystem.
enum class MouseButton : std::uint8_t {
    Left = 0,
    Right,
    Middle,
    Extra1,
    Extra2,
    Count,
};

/// \brief Lightweight 2D vector used to report cursor and scroll deltas.
struct Vector2 {
    float x{0.0F};
    float y{0.0F};
};

/// \brief Tracks transient and persistent input state across frames.
class ENGINE_PLATFORM_API InputState {
public:
    InputState() noexcept;

    /// \brief Resets all stored state to its default values.
    void reset() noexcept;

    /// \brief Captures the start of a new frame, preserving the previous state.
    void begin_frame() noexcept;

    /// \brief Applies a key press/release event to the current frame.
    void apply_key_event(Key key, bool pressed) noexcept;

    /// \brief Applies a mouse button press/release event to the current frame.
    void apply_mouse_button_event(MouseButton button, bool pressed) noexcept;

    /// \brief Updates the cursor position and derives the delta relative to the
    /// start of the current frame.
    void apply_cursor_position(float x, float y) noexcept;

    /// \brief Accumulates scroll deltas generated during the current frame.
    void apply_scroll_delta(float x_offset, float y_offset) noexcept;

    [[nodiscard]] bool is_key_down(Key key) const noexcept;
    [[nodiscard]] bool is_key_up(Key key) const noexcept;
    [[nodiscard]] bool was_key_pressed(Key key) const noexcept;
    [[nodiscard]] bool was_key_released(Key key) const noexcept;

    [[nodiscard]] bool is_mouse_button_down(MouseButton button) const noexcept;
    [[nodiscard]] bool is_mouse_button_up(MouseButton button) const noexcept;
    [[nodiscard]] bool was_mouse_button_pressed(MouseButton button) const noexcept;
    [[nodiscard]] bool was_mouse_button_released(MouseButton button) const noexcept;

    [[nodiscard]] Vector2 cursor_position() const noexcept;
    [[nodiscard]] Vector2 cursor_delta() const noexcept;
    [[nodiscard]] Vector2 scroll_delta() const noexcept;

private:
    using KeyStateArray = std::array<bool, static_cast<std::size_t>(Key::Count)>;
    using MouseStateArray = std::array<bool, static_cast<std::size_t>(MouseButton::Count)>;

    static constexpr std::size_t key_index(Key key) noexcept {
        return static_cast<std::size_t>(key);
    }

    static constexpr std::size_t mouse_index(MouseButton button) noexcept {
        return static_cast<std::size_t>(button);
    }

    KeyStateArray current_keys_{};
    KeyStateArray previous_keys_{};

    MouseStateArray current_mouse_buttons_{};
    MouseStateArray previous_mouse_buttons_{};

    Vector2 cursor_position_{};
    Vector2 cursor_reference_{};
    Vector2 cursor_delta_{};

    Vector2 scroll_delta_{};
};

}  // namespace engine::platform::input

