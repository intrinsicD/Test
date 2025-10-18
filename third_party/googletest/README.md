# Googletest Integration

_Path: `third_party/googletest`_

_Last updated: 2025-02-17_

## Overview

This directory wires the upstream [GoogleTest](https://github.com/google/googletest)
project into the build using CMake's `FetchContent` helper. We track the
`v1.15.2` release so the full fixture API (`TEST_F`, typed tests, parameterised
suites, etc.) is available to the engine's unit and integration test targets.

Key configuration details:

- `BUILD_GMOCK` and `INSTALL_GTEST` are disabled; we only require the GoogleTest
  libraries (`gtest`, `gtest_main`).
- `gtest_force_shared_crt` is forced `ON` so Windows builds link against the
  same CRT model as the rest of the engine.
- The upstream targets are promoted to `GTest::gtest` and
  `GTest::gtest_main` aliases and inherit `engine::project_options` so compiler
  defaults match our modules.

## Usage

When `BUILD_TESTING` is enabled, the root `CMakeLists.txt` adds this directory.
Configuration will fetch the release from GitHub (shallow clone) and expose the
standard `gtest`/`gtest_main` targets. Link your test targets against
`GTest::gtest` or `GTest::gtest_main` as needed.

## License

GoogleTest is distributed under the BSD 3-Clause license. See the upstream
[LICENSE](https://github.com/google/googletest/blob/v1.15.2/LICENSE) file for
details.
