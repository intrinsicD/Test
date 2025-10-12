# Engine Architecture Analysis & Improvement Plan

This document consolidates high-priority architectural corrections and forward-looking improvements required to modularize the engine, decouple optional subsystems, and establish foundations for future milestones. Each item follows the standardized agent processing format to simplify dependency tracking and execution planning.

## Critical Design Corrections

### DC-001: Runtime Module Dependency Inversion
- **Priority:** HIGH
- **Module:** Runtime
- **Dependencies:** []
- **Tasks:**
  - [x] Introduce an `ISubsystemInterface` abstraction in the Core module to formalize subsystem capabilities.
  - [x] Refactor `RuntimeHost` to accept subsystem plugins via dependency injection, removing direct module references.
  - [x] Implement a `SubsystemRegistry` to discover and optionally load subsystem plugins at startup.
  - [x] Update `engine/runtime/src/api.cpp` to expose the plugin-based initialization surface.
  - [x] Add `ENABLE_<MODULE>` build-time flags for each optional subsystem.
  - [x] Document the subsystem plugin contract in `docs/design/plugin_architecture.md`.
- **Artifacts:** `engine/core/include/engine/core/ISubsystemInterface.hpp`, `engine/runtime/include/engine/runtime/runtime_host.hpp`, `engine/runtime/src/runtime_host.cpp`, `engine/runtime/src/api.cpp`, `engine/runtime/src/subsystem_registry.cpp`, `docs/design/plugin_architecture.md`, `CMakeLists.txt` updates for feature flags.
- **Tests:** Runtime initialization smoke tests covering combinations of enabled subsystems; CI preset that exercises CPU-only runtime configuration.
- **Docs:** Update runtime README with plugin usage; extend build documentation to reference the new flags.

### DC-002: CUDA Optional Dependency
- **Priority:** HIGH
- **Module:** Compute
- **Dependencies:** [DC-001]
- **Tasks:**
  - [ ] Add a CMake option `ENGINE_ENABLE_CUDA` (default `OFF`).
  - [ ] Guard the `engine_compute_cuda` target behind the new option.
  - [ ] Implement `engine_compute::cpu_dispatcher` and `engine_compute::cuda_dispatcher` interfaces with a shared abstraction.
  - [ ] Add runtime capability detection helpers in `engine/compute/api.hpp`.
  - [ ] Update build presets in `scripts/build/presets/` to include CUDA-enabled and CPU-only variants.
  - [ ] Document CUDA setup and fallback semantics in `engine/compute/README.md`.
- **Artifacts:** `CMakeLists.txt`, `engine/compute/api.hpp`, `engine/compute/src/dispatchers/*.cpp`, `scripts/build/presets/*.json`, `engine/compute/README.md`.
- **Tests:** Build verification with `ENGINE_ENABLE_CUDA=OFF` and `ON`; runtime unit tests ensuring CPU fallback when CUDA is unavailable; CI preset for CPU-only configuration.
- **Docs:** Expand compute module README; update root README with the new build option.

### DC-003: Platform Backend Abstraction
- **Priority:** MEDIUM
- **Module:** Platform
- **Dependencies:** []
- **Tasks:**
  - [ ] Introduce a configurable CMake option `ENGINE_WINDOW_BACKEND` supporting `GLFW`, `SDL`, and `MOCK` values.
  - [ ] Implement a factory pattern in `engine/platform/windowing/window_system.cpp` that instantiates the requested backend.
  - [ ] Complete the SDL implementation in `engine/platform/src/windowing/sdl_window.cpp` and ensure parity with the GLFW path.
  - [ ] Add backend capability queries to `WindowConfig`.
  - [ ] Extend the test matrix to cover all backend permutations.
  - [ ] Document backend selection and supported features in `docs/modules/platform/README.md`.
- **Artifacts:** `engine/platform/windowing/window_system.cpp`, `engine/platform/src/windowing/sdl_window.cpp`, `engine/platform/include/engine/platform/window_config.hpp`, `CMakeLists.txt`, `docs/modules/platform/README.md`.
- **Tests:** Integration tests per backend, selectable via CTest labels; mock backend smoke test for CI.
- **Docs:** Platform module README updates and root build instructions referencing backend selection.

### DC-004: Error Handling Standardization
- **Priority:** MEDIUM
- **Module:** Cross-cutting
- **Dependencies:** []
- **Tasks:**
  - [ ] Define `engine::Result<T, Error>` in `engine/core/diagnostics/result.hpp` and design the supporting type utilities.
  - [ ] Create an `engine::ErrorCode` hierarchy in `engine/core/diagnostics/error.hpp` that modules can specialize.
  - [ ] Document the error-handling policy in `CODING_STYLE.md` and supporting design docs.
  - [ ] Migrate the IO module to the `Result<T>` pattern as a pilot implementation.
  - [ ] Provide module templates demonstrating correct usage.
  - [ ] Draft a migration guide for existing subsystems.
- **Artifacts:** `engine/core/diagnostics/result.hpp`, `engine/core/diagnostics/error.hpp`, `engine/io` implementation updates, module templates, documentation in `docs/design/error_handling_migration.md`.
- **Tests:** IO module regression suite updated to assert `Result<T>` handling; static analysis or clang-tidy checks for legacy patterns.
- **Docs:** Update coding standards and IO README; provide migration guide references in module docs.

## Architecture Improvements

### AI-001: Resource Lifetime Management
- **Priority:** HIGH
- **Module:** Rendering, Assets
- **Dependencies:** [DC-004]
- **Tasks:**
  - [ ] Design a resource handle system with generation counters.
  - [ ] Implement `ResourcePool<T>` in `engine/core/memory/resource_pool.hpp`.
  - [ ] Provide `ResourceHandle<T>` wrappers with validation logic in `engine/assets/handles.hpp`.
  - [ ] Document ownership patterns in `docs/design/resource_management.md`.
  - [ ] Refactor asset caches to use typed handles.
  - [ ] Add lifetime validation hooks active in debug builds.
- **Artifacts:** `engine/core/memory/resource_pool.hpp`, `engine/assets/handles.hpp`, asset cache sources, `docs/design/resource_management.md`.
- **Tests:** Rendering and asset unit tests covering handle recycling; debug-only assertions verifying stale handle detection.
- **Docs:** Asset and rendering module READMEs updated with the ownership model.

### AI-002: Async Asset Streaming Architecture
- **Priority:** MEDIUM
- **Module:** Assets, IO, Runtime
- **Dependencies:** [AI-001, DC-001]
- **Tasks:**
  - [ ] Author the async loading API design in `docs/design/async_streaming.md`.
  - [ ] Implement `AssetLoadRequest` and `AssetLoadFuture` primitives in the Assets module.
  - [ ] Provide a background thread pool in Core dedicated to IO workloads.
  - [ ] Extend asset caches with a streaming state machine.
  - [ ] Introduce a priority queue scheduling load requests.
  - [ ] Emit streaming metrics to runtime telemetry.
  - [ ] Document best practices in the Assets README.
- **Artifacts:** `docs/design/async_streaming.md`, `engine/assets/include/engine/assets/async.hpp`, `engine/assets/src/streaming/*.cpp`, `engine/core/threading/thread_pool.hpp`, telemetry hooks.
- **Tests:** Asynchronous loading integration tests with deterministic fixtures; runtime telemetry validation.
- **Docs:** Assets README streaming section; runtime telemetry overview.

### AI-003: Frame Graph Resource Metadata
- **Priority:** HIGH
- **Module:** Rendering
- **Dependencies:** []
- **Tasks:**
  - [ ] Extend `FrameGraphResourceInfo` with `ResourceFormat`, `ResourceDimension`, `UsageFlags`, `InitialState`, and `FinalState` descriptors.
  - [ ] Update `RenderPass` definitions to include queue affinity.
  - [ ] Add metadata validation within frame graph compilation.
  - [ ] Provide a migration guide for existing pass definitions.
  - [ ] Update unit tests in `engine/rendering/tests/test_frame_graph.cpp` to cover new metadata.
- **Artifacts:** `engine/rendering/frame_graph/frame_graph.hpp`, `engine/rendering/frame_graph/render_pass.hpp`, `engine/rendering/tests/test_frame_graph.cpp`, `docs/design/frame_graph_metadata_migration.md`.
- **Tests:** Frame graph unit tests; backend smoke tests validating barrier generation.
- **Docs:** Rendering module README referencing metadata requirements; migration guide for downstream teams.

### AI-004: Compute Kernel Dependency Validation
- **Priority:** MEDIUM
- **Module:** Compute
- **Dependencies:** [DC-002]
- **Tasks:**
  - [ ] Implement Tarjan's strongly connected components algorithm in `kernel_dispatcher.cpp` to detect cycles.
  - [ ] Invoke `validate_dependencies()` prior to dispatch.
  - [ ] Emit a `CyclicDependencyException` with a readable cycle path.
  - [ ] Extend compute tests to cover cycle detection scenarios.
  - [ ] Document dependency requirements in the Compute README.
- **Artifacts:** `engine/compute/src/kernel_dispatcher.cpp`, `engine/compute/include/engine/compute/kernel_dispatcher.hpp`, `engine/compute/tests/`, README updates.
- **Tests:** Unit tests constructing cyclic graphs; integration test ensuring dispatch succeeds when DAG is valid.
- **Docs:** Compute README detailing dependency validation.

## Roadmap Advancement TODOs

### RT-001: Animation Deformation Pipeline (M3)
- **Priority:** HIGH
- **Module:** Animation
- **Dependencies:** Geometry module updates
- **Tasks:**
  - [ ] Design a `RigBinding` structure mapping joints to vertex groups.
  - [ ] Implement `bind_rig_to_mesh()` in `engine/animation/src/deformation.cpp`.
  - [ ] Add linear blend skinning in `engine/animation/src/skinning.cpp`.
  - [ ] Create an integration test with a procedural mesh and 3-joint rig.
  - [ ] Benchmark skinning performance targeting 10K vertices at 60 FPS.
  - [ ] Document the skinning API in `docs/api/animation.md`.
  - [ ] Update the Animation ROADMAP with Phase 2 checklist status.
- **Artifacts:** Animation source files, procedural test assets, benchmarks, documentation updates.
- **Tests:** Integration tests for deformation; performance benchmarks recorded in CI artifacts.
- **Docs:** Animation API documentation and roadmap updates.

### RT-002: Physics Contact Manifolds (M3-M4)
- **Priority:** HIGH
- **Module:** Physics
- **Dependencies:** Geometry intersection tests
- **Tasks:**
  - [ ] Design `ContactManifold` in `engine/physics/collision/contact.hpp`.
  - [ ] Implement persistent contact tracking across frames.
  - [ ] Add contact point generation for sphere-sphere, sphere-box, and box-box cases.
  - [ ] Build an impulse-based constraint solver in `engine/physics/dynamics/solver.cpp`.
  - [ ] Extend `RigidBody` with restitution and friction parameters.
  - [ ] Write integration tests covering stacking scenarios.
  - [ ] Profile solver performance and document findings.
- **Artifacts:** Physics collision and dynamics sources, benchmarking scripts, docs.
- **Tests:** Regression suite for collision manifolds; performance profiling outputs.
- **Docs:** Physics module README and roadmap sections updated.

### RT-003: Rendering Backend Prototype – Vulkan (M4)
- **Priority:** HIGH
- **Module:** Rendering
- **Dependencies:** [AI-003]
- **Tasks:**
  - [ ] Implement `VulkanResourceProvider` in `engine/rendering/backend/vulkan/`.
  - [ ] Provide device, queue, and swapchain initialization routines.
  - [ ] Add resource allocation logic for buffers, images, and samplers.
  - [ ] Implement `VulkanGpuScheduler` that translates frame graph passes into command buffers.
  - [ ] Generate barriers from resource state metadata.
  - [ ] Create a “Hello Triangle” integration test.
  - [ ] Document Vulkan backend setup in `docs/modules/rendering/backend/vulkan/README.md`.
- **Artifacts:** Vulkan backend sources, integration tests, documentation.
- **Tests:** Integration test verifying swapchain rendering; unit tests for resource translation.
- **Docs:** Backend README and root build instructions for Vulkan prerequisites.

### RT-004: Runtime Lifecycle Diagnostics (M3)
- **Priority:** HIGH
- **Module:** Runtime
- **Dependencies:** Core diagnostics subsystem
- **Tasks:**
  - [ ] Add structured logging to `RuntimeHost::initialize()`.
  - [ ] Log subsystem initialization order with timings.
  - [ ] Implement `RuntimeHost::get_diagnostics()` exposing subsystem health, kernel timings, and memory usage stats.
  - [ ] Add a repeated init/shutdown test to detect leaks.
  - [ ] Build a diagnostics dashboard in `engine/tools/diagnostics/`.
  - [ ] Document the diagnostics API in the Runtime README.
- **Artifacts:** Runtime diagnostics code, dashboard tooling, documentation.
- **Tests:** Automated lifecycle tests; tooling smoke tests capturing telemetry output.
- **Docs:** Runtime README and tooling guides.

### RT-005: Scene Hierarchy Validation (M3)
- **Priority:** HIGH
- **Module:** Scene
- **Dependencies:** None
- **Tasks:**
  - [ ] Implement `detect_hierarchy_cycles()` in `engine/scene/systems/hierarchy_system.cpp`.
  - [ ] Provide `validate_scene()` ensuring no orphaned children, required transforms, and optional name uniqueness.
  - [ ] Integrate validation into scene serialization flows.
  - [ ] Add tests with intentionally broken hierarchies.
  - [ ] Expose validation reports through the C API.
- **Artifacts:** Scene system sources, serialization updates, C API bindings, documentation.
- **Tests:** Unit and integration tests covering hierarchy anomalies.
- **Docs:** Scene module README and API documentation updates.

### RT-006: IO Format Detection Hardening (M3)
- **Priority:** HIGH
- **Module:** IO
- **Dependencies:** None
- **Tasks:**
  - [ ] Implement magic-number detection in `detect_geometry_file()`.
  - [ ] Maintain a format signature database in `engine/io/src/format_signatures.cpp`.
  - [ ] Add fuzzing tests using malformed files.
  - [ ] Introduce structured error reporting via `Result<Format, IOError>`.
  - [ ] Document supported formats with signature examples.
  - [ ] Update the IO README with detection algorithm specifics.
- **Artifacts:** IO detection sources, fuzzing harnesses, documentation.
- **Tests:** Fuzzing jobs integrated into CI; deterministic unit tests for known signatures.
- **Docs:** IO README and detection documentation.

## Documentation Improvements

### DI-001: Module README Standardization
- **Priority:** MEDIUM
- **Module:** Documentation
- **Dependencies:** []
- **Tasks:**
  - [ ] Audit module READMEs against `docs/README_TEMPLATE.md`.
  - [ ] Ensure each README contains Dependencies, Code Examples, Troubleshooting, and See Also sections.
  - [ ] Add cross-links between related modules.
  - [ ] Update `scripts/validate_docs.py` to check README completeness.
- **Artifacts:** Updated module READMEs, documentation validation script.
- **Tests:** Documentation validation script executed in CI.
- **Docs:** Root README referencing the standardization process.

### DI-002: API Reference Generation
- **Priority:** MEDIUM
- **Module:** Documentation
- **Dependencies:** []
- **Tasks:**
  - [ ] Add a Doxygen configuration at the repository root.
  - [ ] Annotate public Math headers with Doxygen comments as a pilot.
  - [ ] Generate reference output under `docs/api/generated/`.
  - [ ] Provide a `make docs` CMake target.
  - [ ] Publish generated docs as CI artifacts.
  - [ ] Update `docs/api/README.md` with generation instructions.
- **Artifacts:** `Doxyfile`, annotated headers, generated docs, CMake updates.
- **Tests:** CI job invoking documentation generation.
- **Docs:** API README and build documentation.

### DI-003: Architecture Decision Records
- **Priority:** LOW
- **Module:** Documentation
- **Dependencies:** []
- **Tasks:**
  - [ ] Create `docs/design/adr/` with an ADR template.
  - [ ] Document ADR-001 through ADR-004 covering ECS choice, frame graph rationale, C API strategy, and module boundaries.
  - [ ] Cross-reference ADRs from module READMEs.
- **Artifacts:** ADR markdown files, template.
- **Tests:** Documentation linting to ensure ADR links resolve.
- **Docs:** Module READMEs referencing ADRs.

## Build System Enhancements

### BS-001: Preset Expansion
- **Priority:** MEDIUM
- **Module:** Build System
- **Dependencies:** []
- **Tasks:**
  - [ ] Add presets for `linux-clang-release`, `windows-clang-debug`, `macos-appleclang-debug`, a minimal Math+Core configuration, and a full CUDA build.
  - [ ] Document preset usage in the root README.
- **Artifacts:** `CMakePresets.json`, scripts in `scripts/build/presets/`, README updates.
- **Tests:** CI matrix entries verifying the new presets.
- **Docs:** Root README preset section.

### BS-002: Versioning Strategy
- **Priority:** MEDIUM
- **Module:** Core, Build System
- **Dependencies:** []
- **Tasks:**
  - [ ] Introduce version macros in `engine/core/version.hpp`.
  - [ ] Implement semantic version compatibility checks during module loading.
  - [ ] Add `version.cmake` with `project(Engine VERSION 0.1.0)` and propagate to shared library names.
  - [ ] Document versioning policy in `docs/design/versioning.md`.
- **Artifacts:** Version header, CMake scripts, documentation.
- **Tests:** Module loading tests verifying version mismatches.
- **Docs:** Versioning design document and root README version policy.

### BS-003: CMake Target Cleanup
- **Priority:** LOW
- **Module:** Build System
- **Dependencies:** []
- **Tasks:**
  - [ ] Remove unused CMake variables flagged by `cmake --warn-unused-vars`.
  - [ ] Consolidate `engine_apply_module_defaults()` usage.
  - [ ] Add INTERFACE targets for header-only utilities.
  - [ ] Document best practices in `docs/design/cmake_patterns.md`.
- **Artifacts:** `CMakeLists.txt` cleanups, documentation.
- **Tests:** Configure with warnings enabled to ensure clean runs.
- **Docs:** CMake patterns design note.

## Testing Infrastructure

### TI-001: Integration Test Suite
- **Priority:** HIGH
- **Module:** Testing
- **Dependencies:** [DC-001, AI-001]
- **Tasks:**
  - [ ] Create `engine/tests/integration/` structure with `animation_physics/`, `io_assets/`, and `runtime_rendering/` subdirectories.
  - [ ] Implement deterministic fixtures with fixed seeds.
  - [ ] Enable `ctest --label-regex integration` selection.
  - [ ] Document integration test guidelines.
- **Artifacts:** Integration test sources, fixtures, documentation.
- **Tests:** Integration test executions across configurations.
- **Docs:** `engine/tests/integration/README.md` and testing overview updates.

### TI-002: Performance Benchmarks
- **Priority:** MEDIUM
- **Module:** Testing
- **Dependencies:** []
- **Tasks:**
  - [ ] Add Google Benchmark to `third_party/`.
  - [ ] Provide benchmark sources for Animation, Physics, and Geometry.
  - [ ] Add a `make benchmark` target.
  - [ ] Configure CI regression detection (20% slowdown threshold).
- **Artifacts:** Benchmark sources, build scripts, CI configuration.
- **Tests:** Automated benchmark runs comparing against baselines.
- **Docs:** Testing README documenting benchmark workflow.

### TI-003: Fuzzing Harnesses
- **Priority:** LOW
- **Module:** Testing, IO
- **Dependencies:** [RT-006]
- **Tasks:**
  - [ ] Add libFuzzer-based targets for IO parsers.
  - [ ] Maintain a corpus under `engine/tests/fuzzing/corpus/`.
  - [ ] Optionally integrate with OSS-Fuzz.
  - [ ] Document fuzzing workflow.
- **Artifacts:** Fuzzing targets, corpus, documentation.
- **Tests:** Periodic fuzzing jobs; local harness execution instructions.
- **Docs:** Testing README fuzzing section.

## Python Bindings Completeness

### PY-001: Core Bindings
- **Priority:** MEDIUM
- **Module:** Python Bindings
- **Dependencies:** [DC-004, AI-001]
- **Tasks:**
  - [ ] Document binding generation in `python/README.md`.
  - [ ] Add `pybind11` to `requirements.txt`.
  - [ ] Expose Animation and Geometry APIs via `python/engine3g/animation.py` and `python/engine3g/geometry.py`.
  - [ ] Provide `.pyi` stubs for IDE support.
  - [ ] Implement `pytest` coverage in `python/tests/test_bindings.py`.
- **Artifacts:** Python binding modules, requirements, tests, documentation.
- **Tests:** Pytest suite covering bindings; linting for type hints.
- **Docs:** Python README and API references.

## Cross-Cutting Initiatives

### CC-001: Telemetry Framework
- **Priority:** MEDIUM
- **Module:** Core, Runtime, All
- **Dependencies:** [RT-004]
- **Tasks:**
  - [ ] Design telemetry API in `engine/core/diagnostics/telemetry.hpp`.
  - [ ] Implement event recording with timestamps and scoped profiling macros (`ENGINE_PROFILE_SCOPE`).
  - [ ] Provide telemetry sinks (JSON export, live viewer).
  - [ ] Instrument Animation, Physics, and Rendering hot paths.
  - [ ] Build a telemetry viewer in `engine/tools/profiling/viewer/`.
- **Artifacts:** Telemetry headers, runtime instrumentation, profiling tool, documentation.
- **Tests:** Unit tests for telemetry buffering; integration tests verifying viewer ingestion.
- **Docs:** Telemetry API reference and tooling README.

### CC-002: Hot Reload Infrastructure
- **Priority:** MEDIUM
- **Module:** Assets, Platform, Tools
- **Dependencies:** [AI-001]
- **Tasks:**
  - [ ] Implement file watching in `engine/platform/filesystem/watcher.cpp`.
  - [ ] Add callback registration to asset caches.
  - [ ] Design a reload transaction log handling failures.
  - [ ] Add an integration test that modifies a file and validates live updates.
  - [ ] Document workflows in the Assets README.
- **Artifacts:** Filesystem watcher, asset cache updates, integration tests, documentation.
- **Tests:** Automated hot-reload scenario tests.
- **Docs:** Assets README and tooling notes.

## Milestone Coordination

### MC-001: Milestone Dashboard
- **Priority:** LOW
- **Module:** Project Management
- **Dependencies:** []
- **Tasks:**
  - [ ] Create `docs/milestones/M3.md` aggregating module tasks, dependency graphs, and acceptance criteria.
  - [ ] Apply milestone labels to GitHub issues.
  - [ ] Automate milestone progress tracking.
- **Artifacts:** Milestone documentation, automation scripts.
- **Tests:** Documentation lints; automation dry runs.
- **Docs:** Milestone README references.

### MC-002: Synchronize Module Roadmaps
- **Priority:** MEDIUM
- **Module:** Project Management
- **Dependencies:** [DI-001]
- **Tasks:**
  - [ ] Cross-reference module ROADMAPs with the central roadmap, flagging orphaned TODOs.
  - [ ] Add "Last Updated" metadata to each roadmap file.
  - [ ] Extend `scripts/validate_docs.py` to check roadmap consistency.
- **Artifacts:** Updated roadmap documents, validation script enhancements.
- **Tests:** Documentation validation in CI.
- **Docs:** Roadmap maintenance guidelines.

## Priority Summary

| Horizon | Rank | Item |
|---------|------|------|
| Sprint 1 | 1 | DC-001 |
|         | 2 | DC-002 |
|         | 3 | AI-003 |
|         | 4 | RT-001 |
|         | 5 | TI-001 |
| Sprint 2-3 | 1 | DC-004 |
|            | 2 | AI-001 |
|            | 3 | RT-002 |
|            | 4 | RT-004 |
|            | 5 | DI-001 |
| M4-M5 | 1 | AI-002 |
|       | 2 | RT-003 |
|       | 3 | CC-001 |
|       | 4 | PY-001 |

This plan should be revisited at the start of each milestone to adjust priorities, confirm dependencies, and align with evolving product goals.
