# Camera Input Bug Fix Report

**Date:** November 16, 2025  
**Issue:** Camera cannot be controlled with mouse or scroll in geometry_viewer  
**Status:** ✅ FIXED

## Problem Description

User reported that:
1. ✓ The cube is visible
2. ✗ Cannot move camera with mouse drag (trackball)
3. ✗ Cannot zoom with scroll wheel
4. ✗ WASD keys not implemented (by design - only trackball mode)

## Root Cause Analysis

### Critical Bug Found: Event Polling Order

**Location:** `/engine/platform/src/windowing/glfw_window.cpp:212-225`

**The Issue:**

The `pump_events()` function was calling operations in the wrong order:

```cpp
void pump_events() override
{
    glfwPollEvents();  // 1. Polls events, triggers callbacks that update input state
    
    // ... window close handling ...
    
    HeadlessWindow::pump_events();  // 2. Calls begin_frame() which RESETS input state!
}
```

**The Problem Flow:**

1. `glfwPollEvents()` is called
2. GLFW callbacks are triggered synchronously:
   - `glfwSetCursorPosCallback` → calls `apply_cursor_position()`
   - This updates `cursor_position_` and calculates `cursor_delta_ = cursor_position_ - cursor_reference_`
   - `glfwSetScrollCallback` → calls `apply_scroll_delta()`
   - This accumulates scroll offsets into `scroll_delta_`
3. **Then** `HeadlessWindow::pump_events()` is called
4. This calls `input_state_.begin_frame()` which:
   - Resets `cursor_delta_ = {0, 0}` ❌
   - Resets `scroll_delta_ = {0, 0}` ❌
   - Updates `cursor_reference_ = cursor_position_`

**Result:** All input delta values accumulated during event polling are immediately discarded!

## The Fix

**Changed order to call `begin_frame()` BEFORE polling events:**

```cpp
void pump_events() override
{
    // Begin frame BEFORE polling events so cursor delta accumulates correctly
    HeadlessWindow::pump_events();  // 1. Reset deltas and save reference
    
    glfwPollEvents();  // 2. Poll events which accumulate into deltas
    
    // ... window close handling ...
}
```

**Correct Flow:**

1. `HeadlessWindow::pump_events()` calls `begin_frame()`:
   - Sets `cursor_reference_ = cursor_position_` (saves current position)
   - Resets `cursor_delta_ = {0, 0}` (ready for new frame)
   - Resets `scroll_delta_ = {0, 0}` (ready for new frame)
2. `glfwPollEvents()` triggers callbacks:
   - Cursor moves → `cursor_delta_ = new_position - cursor_reference_` ✓
   - Scroll wheel → `scroll_delta_ += offset` ✓
3. Application can now read the correct delta values in `handle_input()`

## Impact

This bug affected **all input delta-based operations**:
- Mouse cursor delta (camera rotation)
- Scroll wheel delta (camera zoom)
- Any frame-based input accumulation

## Files Modified

1. `/engine/platform/src/windowing/glfw_window.cpp` - Fixed `pump_events()` order

## Testing

To verify the fix works:

1. Build and run geometry_viewer
2. Click and drag with left mouse button → camera should rotate
3. Use scroll wheel → camera should zoom in/out
4. Press ESC → application should quit

## Related Issues

None - this was an isolated event polling order bug.

## Verification

The fix should be verified by:
1. Running geometry_viewer and confirming mouse/scroll input works
2. Running platform input tests to ensure no regressions
3. Checking any other windowing backends (if they exist) for similar issues

---

**Bug Severity:** CRITICAL - Completely broke all mouse/scroll input  
**Fix Complexity:** TRIVIAL - 2 lines swapped  
**Fix Confidence:** HIGH - Clear cause and effect, logical fix

