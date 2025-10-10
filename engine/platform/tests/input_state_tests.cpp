#include <gtest/gtest.h>

#include "engine/platform/input/input_state.hpp"

namespace {

using namespace engine::platform::input;

TEST(InputState, TracksKeyTransitions) {
    InputState state{};
    state.begin_frame();

    state.apply_key_event(Key::Space, true);
    EXPECT_TRUE(state.is_key_down(Key::Space));
    EXPECT_FALSE(state.is_key_up(Key::Space));
    EXPECT_TRUE(state.was_key_pressed(Key::Space));
    EXPECT_FALSE(state.was_key_released(Key::Space));

    state.begin_frame();
    EXPECT_TRUE(state.is_key_down(Key::Space));
    EXPECT_FALSE(state.was_key_pressed(Key::Space));
    EXPECT_FALSE(state.was_key_released(Key::Space));

    state.apply_key_event(Key::Space, false);
    EXPECT_FALSE(state.is_key_down(Key::Space));
    EXPECT_TRUE(state.is_key_up(Key::Space));
    EXPECT_TRUE(state.was_key_released(Key::Space));

    state.begin_frame();
    EXPECT_FALSE(state.was_key_released(Key::Space));
}

TEST(InputState, TracksMouseButtonTransitions) {
    InputState state{};
    state.begin_frame();

    state.apply_mouse_button_event(MouseButton::Left, true);
    EXPECT_TRUE(state.is_mouse_button_down(MouseButton::Left));
    EXPECT_TRUE(state.was_mouse_button_pressed(MouseButton::Left));
    EXPECT_FALSE(state.was_mouse_button_released(MouseButton::Left));

    state.begin_frame();
    EXPECT_TRUE(state.is_mouse_button_down(MouseButton::Left));
    EXPECT_FALSE(state.was_mouse_button_pressed(MouseButton::Left));

    state.apply_mouse_button_event(MouseButton::Left, false);
    EXPECT_FALSE(state.is_mouse_button_down(MouseButton::Left));
    EXPECT_TRUE(state.was_mouse_button_released(MouseButton::Left));

    state.begin_frame();
    EXPECT_FALSE(state.was_mouse_button_released(MouseButton::Left));
}

TEST(InputState, ReportsCursorPositionAndDelta) {
    InputState state{};
    state.begin_frame();

    state.apply_cursor_position(10.0F, 20.0F);
    auto position = state.cursor_position();
    EXPECT_FLOAT_EQ(position.x, 10.0F);
    EXPECT_FLOAT_EQ(position.y, 20.0F);

    auto delta = state.cursor_delta();
    EXPECT_FLOAT_EQ(delta.x, 10.0F);
    EXPECT_FLOAT_EQ(delta.y, 20.0F);

    state.apply_cursor_position(15.0F, 18.0F);
    delta = state.cursor_delta();
    EXPECT_FLOAT_EQ(delta.x, 15.0F);
    EXPECT_FLOAT_EQ(delta.y, 18.0F);

    state.begin_frame();
    delta = state.cursor_delta();
    EXPECT_FLOAT_EQ(delta.x, 0.0F);
    EXPECT_FLOAT_EQ(delta.y, 0.0F);

    state.apply_cursor_position(12.0F, 8.0F);
    delta = state.cursor_delta();
    EXPECT_FLOAT_EQ(delta.x, -3.0F);
    EXPECT_FLOAT_EQ(delta.y, -10.0F);
}

TEST(InputState, AccumulatesScrollDelta) {
    InputState state{};
    state.begin_frame();

    state.apply_scroll_delta(1.0F, -2.0F);
    state.apply_scroll_delta(0.5F, 0.25F);
    auto scroll = state.scroll_delta();
    EXPECT_FLOAT_EQ(scroll.x, 1.5F);
    EXPECT_FLOAT_EQ(scroll.y, -1.75F);

    state.begin_frame();
    scroll = state.scroll_delta();
    EXPECT_FLOAT_EQ(scroll.x, 0.0F);
    EXPECT_FLOAT_EQ(scroll.y, 0.0F);
}

TEST(InputState, ResetClearsState) {
    InputState state{};
    state.begin_frame();

    state.apply_key_event(Key::E, true);
    state.apply_mouse_button_event(MouseButton::Right, true);
    state.apply_cursor_position(5.0F, 6.0F);
    state.apply_scroll_delta(2.0F, 3.0F);

    state.reset();
    EXPECT_FALSE(state.is_key_down(Key::E));
    EXPECT_FALSE(state.was_key_pressed(Key::E));
    EXPECT_FALSE(state.is_mouse_button_down(MouseButton::Right));
    EXPECT_FALSE(state.was_mouse_button_pressed(MouseButton::Right));

    auto position = state.cursor_position();
    EXPECT_FLOAT_EQ(position.x, 0.0F);
    EXPECT_FLOAT_EQ(position.y, 0.0F);

    auto delta = state.cursor_delta();
    EXPECT_FLOAT_EQ(delta.x, 0.0F);
    EXPECT_FLOAT_EQ(delta.y, 0.0F);

    auto scroll = state.scroll_delta();
    EXPECT_FLOAT_EQ(scroll.x, 0.0F);
    EXPECT_FLOAT_EQ(scroll.y, 0.0F);
}

}  // namespace

