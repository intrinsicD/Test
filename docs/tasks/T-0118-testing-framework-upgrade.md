# T-0118: Restore Googletest Fixture Support

## Goal
Replace the stubbed Googletest headers with a fixture-capable build so the
`engine/tests/*` suites (especially `engine/tests/integration`) compile and run
again. This unblocks roadmap item `TI-001` and keeps future test additions from
re-implementing testing primitives.

## Context
- The vendored header at `third_party/googletest/include/gtest/gtest.h`
  re-implements a minimal subset of Googletest and omits the `TEST_F` macro and
  fixture plumbing entirely. Files such as
  `engine/assets/tests/test_async.cpp` rely on `TEST_F`, causing compilation to
  abort when `cmake --build --preset linux-gcc-debug` reaches those targets.
- Build failure excerpt:
  ```text
  /workspace/Test/engine/assets/tests/test_async.cpp:135:28: error: ‘LoadAsyncCompletesSuccessfully’ has not been declared
    135 | TEST_F(MeshCacheAsyncTest, LoadAsyncCompletesSuccessfully)
        |                            ^~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  ```
- Preprocessing confirms the macro is left untouched, and the header lacks any
  `TEST_F` definition. The integration harness therefore cannot be compiled,
  blocking `TI-001` acceptance criteria and the sprint-06 goal.

## Inputs
- Code: `third_party/googletest/`, `engine/tests/**`, `CMakeLists.txt` (root and
  per-module test targets).
- Docs: [`docs/ROADMAP.md`](../ROADMAP.md#ti-001-integration-suites),
  [`docs/tasks/T-0114-testing-integration-suites.md`](T-0114-testing-integration-suites.md),
  module READMEs that reference `TI-001`.
- Build scripts: presets under `scripts/build/presets/`.

## Deliverables
1. Vendor a real Googletest release (or reintroduce the upstream target via
   `FetchContent`) that exposes the full macro surface, including fixtures.
2. Update CMake to link against the upgraded library while preserving existing
   targets (`GTest::gtest`, `GTest::gtest_main`).
3. Ensure every test target builds with the new headers on Linux/Windows presets
   without additional source changes.
4. Update documentation to record the dependency change and unblock notes in the
   roadmap/task records.

## Acceptance Criteria
- [ ] `cmake --build --preset linux-gcc-debug --target engine_assets_tests`
      succeeds without compile errors from `TEST_F`.
- [ ] `ctest --preset linux-gcc-debug --tests-regex engine_integration_tests`
      runs to completion on the upgraded toolchain.
- [ ] `docs/ROADMAP.md` and `docs/tasks/T-0114-testing-integration-suites.md`
      mark `TI-001` as unblocked.
- [ ] Third-party licensing and README updates document the new Googletest
      source.

## Follow-Up Considerations
- Evaluate whether additional Googletest components (`gmock`, utilities) should
  also be vendored to reduce future friction.
- Decide if the custom minimal harness should be retained for lightweight
  embedded builds or removed entirely once upstream coverage is restored.
