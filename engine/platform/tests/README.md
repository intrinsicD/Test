# Tests

_Path: `engine/platform/tests`_

_Last updated: 2025-10-06_

## Contents

### Files

- `CMakeLists.txt` – Text resource.
- `test_module.cpp` – GoogleTest suite covering module exports, headless behaviour, and the GLFW backend lifecycle (skipped automatically when GLFW cannot initialise).
- `window_console_tests.cpp` – GoogleTest coverage for the reusable interactive console behaviour.
- `window_test_app.cpp` – Interactive console harness for manually exercising window backends and event delivery.
