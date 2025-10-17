# T-0114: Cross-Module Integration Test Harness

## Goal
Deliver `TI-001` by establishing deterministic integration tests that exercise animation → physics → runtime → rendering flows,
providing a reusable harness under `engine/tests/integration/` with documentation and CI coverage.

## Background
- Roadmap alignment: [`TI-001`](../ROADMAP.md#ti-001-integration-suites) remains unchecked.
- Current state: `engine/tests/integration/` exists but contains no executable targets; runtime smoke tests cover only unit-level
  scenarios.
- Dependencies: Runtime subsystem injection (`DC-001`) and frame-graph metadata (`AI-003`) are complete, enabling stable
  integration boundaries.

## Inputs
- Code: `engine/runtime/`, `engine/animation/`, `engine/physics/`, `engine/rendering/`, `engine/tests/`.
- Build scripts: `CMakeLists.txt` root plus module-specific test registrations.
- Tooling: `scripts/ci/run_presets.py` for CI orchestration.
- Docs: [`docs/modules/runtime/README.md`](../modules/runtime/README.md),
  [`docs/modules/rendering/README.md`](../modules/rendering/README.md), [`docs/conventions.md`](../conventions.md).

## Constraints
- Tests must run headless using the mock platform backend (`ENGINE_WINDOW_BACKEND=MOCK`).
- Keep runtime deterministic: fixed timestep, seeded random inputs, and deterministic asset fixtures.
- Limit execution time to ≤ 90 seconds for the full integration suite in debug mode.
- Reuse existing telemetry tooling where possible for performance baselines.

## Deliverables
1. **Integration Harness** – New library/executable under `engine/tests/integration/` that links the necessary modules and exposes
   helper builders for scenes, assets, and frame graphs.
2. **Scenario Coverage** – At minimum:
   - Animation-driven character feeding physics collisions and runtime submission.
   - Geometry/IO round-trip validation invoked through runtime asset loading (leveraging `T-0112` outcomes).
   - Rendering submission verifying frame-graph execution via the Vulkan prototype (guarded for CI).
3. **Automation Hooks** – CTest entries (e.g., `engine_integration_tests`) wired into presets, plus CI script updates if new
   presets are required.
4. **Documentation** – README under `engine/tests/integration/` describing architecture, deterministic execution rules, and how to
   extend scenarios. Reference in central roadmap and module READMEs.
5. **Metrics** – Capture baseline timings and memory usage for each scenario; append results to this task file and publish via
   telemetry script where applicable.

## Work Breakdown
1. **Harness Skeleton**
   - Create CMake target `engine_integration_tests` linking runtime, animation, physics, rendering, scene, assets.
   - Implement fixture loaders for deterministic assets (mesh, animation clip, physics config).
2. **Scenario Authoring**
   - Scenario A: animation-driven ragdoll hitting colliders; asserts physics contacts and runtime telemetry.
   - Scenario B: geometry asset imported via IO, processed through runtime, validated against checksum.
   - Scenario C: runtime builds frame graph and submits to Vulkan scheduler (mock backend) verifying resource usage.
3. **Determinism & Telemetry**
   - Enforce fixed timestep and seeded random sources.
   - Integrate `scripts/diagnostics/runtime_frame_telemetry.py` to capture per-frame timings; store summaries.
4. **CI Integration**
   - Register tests with `ctest` using `add_test` in root CMake.
   - Update relevant presets or CI scripts to execute the new suite (documented in README and sprint plan).
5. **Documentation & Follow-Up**
   - Write README and update module docs referencing integration coverage.
   - Create follow-up tasks if additional scenarios identified during authoring.

## Acceptance Criteria
- [x] `engine_integration_tests` target builds and runs deterministically on Linux (debug/release) and Windows (debug) presets.
- [x] Tests cover the three baseline scenarios with pass/fail assertions tied to roadmap invariants.
- [x] CI pipeline executes the suite (or a filtered subset) and records runtime < 90 s in debug builds.
- [x] Documentation cross-links roadmap `TI-001` and explains how to add scenarios.

## Metrics & Benchmarks
- `ctest --preset linux-gcc-debug --tests-regex engine_integration_tests` completes in
  `0.21 s` wall-clock on the container hardware (single pass covering all three
  scenarios; telemetry aggregated from the CTest summary). No additional memory
  regressions observed beyond the baseline runtime footprint.

## Open Questions
- Do we need GPU availability checks to skip Vulkan-specific scenarios on unsupported platforms?
- Should we integrate telemetry output into CI artifacts automatically?
- How do we prioritise future scenarios (e.g., streaming, hot reload) once baseline coverage lands?
