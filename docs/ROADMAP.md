# Central Roadmap

This roadmap aggregates the actionable plans defined by each engine module and highlights cross-cutting initiatives. Consult the per-module documents for detailed task descriptions.

## Architecture Improvement Plan

The architecture improvement plan captures cross-cutting backlog items that span multiple
modules. Items are grouped by their intent:

- **Critical Design Corrections (DC)** – rectify architectural flaws that block ongoing work.
- **Architecture Improvements (AI)** – introduce new systems or structural capabilities.
- **Roadmap TODOs (RT)** – unlock downstream deliverables by completing feature work.
- **Documentation Improvements (DI)** – raise the baseline documentation quality.
- **Build System Enhancements (BS)** – improve build configuration and dependency hygiene.
- **Testing Infrastructure (TI)** – expand validation and benchmarking coverage.
- **Python Bindings Completeness (PY)** – ensure the Python façade matches the C++ API.
- **Cross-Cutting Initiatives (CC)** – deliver functionality that touches multiple modules.
- **Milestone Coordination (MC)** – maintain planning artefacts and roadmap alignment.

### Outstanding Backlog Focus

Near-term planning should prioritise the following open items:

- **`AI-004` – Compute dependency validation**: add dependency metadata, perform cycle detection, emit diagnostics, and update
  the dispatcher documentation.
- **`RT-002` – Physics contact manifolds**: introduce persistent manifolds, integrate constraint solving hooks, add telemetry,
  and document workflows.
- **`RT-004` – Runtime diagnostics**: instrument lifecycle stages, expose telemetry through `RuntimeHost`, and surface metrics in
  tooling.
- **`RT-005` – Scene hierarchy validation**: provide cycle/transform checks, reporting hooks, and documentation.
- **`RT-006` – IO format detection hardening**: expand signature databases and integrate fuzzing harnesses beyond the structured
  error work already completed.
- **`CC-001` – Telemetry framework**: design the cross-module API, implement sinks, instrument hot paths, and build a profiling
  viewer.
- **`CC-002` – Hot reload infrastructure**: add filesystem watching, asset cache callbacks, transaction logging, integration
  tests, and documentation.
- **`TI-002` & `TI-003` – Benchmarking and fuzzing harnesses**: introduce benchmarks, CI regression detection, libFuzzer targets,
  corpora, and documentation.
- **`PY-001` – Python bindings**: document binding generation, add dependencies, expose animation/geometry APIs, provide stubs,
  and cover them with pytest.
- **`DI-001`–`DI-003` – Documentation improvements**: standardise READMEs, automate completeness checks, generate API references,
  and stand up ADR infrastructure.
- **`BS-001`–`BS-003` – Build system hygiene**: expand presets, codify versioning policy, and audit target dependency hygiene.
- **`MC-001` & `MC-002` – Milestone coordination**: publish milestone dashboards and ensure module roadmaps stay in sync with this
  plan.

### Critical Design Corrections

#### DC-001: Runtime Module Dependency Inversion
- **Priority:** HIGH
- **Module:** Runtime
- **Dependencies:** []
- **Tasks:**
  - [x] Introduce an `ISubsystemInterface` abstraction in the Core module to formalize subsystem capabilities.
  - [x] Refactor `RuntimeHost` to accept subsystem plugins via dependency injection, removing direct module references.
  - [x] Implement a `SubsystemRegistry` to discover and optionally load subsystem plugins at startup.
  - [x] Update `engine/runtime/src/api.cpp` to expose the plugin-based initialization surface.
  - [x] Add `ENABLE_<MODULE>` build-time flags for each optional subsystem.
  - [x] Document the subsystem plugin contract in the runtime module README.
- **Artifacts:** `engine/core/include/engine/core/ISubsystemInterface.hpp`, `engine/runtime/include/engine/runtime/runtime_host.hpp`, `engine/runtime/src/runtime_host.cpp`, `engine/runtime/src/api.cpp`, `engine/runtime/src/subsystem_registry.cpp`, runtime README updates, `CMakeLists.txt` feature flag configuration.
- **Tests:** Runtime initialization smoke tests covering combinations of enabled subsystems; CI preset that exercises CPU-only runtime configuration.
- **Docs:** Update runtime README with plugin usage; extend build documentation to reference the new flags.

#### DC-002: CUDA Optional Dependency
- **Priority:** HIGH
- **Module:** Compute
- **Dependencies:** [DC-001]
- **Tasks:**
  - [x] Add a CMake option `ENGINE_ENABLE_CUDA` (default `OFF`).
  - [x] Guard the `engine_compute_cuda` target behind the new option.
  - [x] Implement `engine_compute::cpu_dispatcher` and `engine_compute::cuda_dispatcher` interfaces with a shared abstraction.
  - [x] Add runtime capability detection helpers in `engine/compute/api.hpp`.
  - [x] Update build presets in `scripts/build/presets/` to include CUDA-enabled and CPU-only variants.
  - [x] Document CUDA setup and fallback semantics in `engine/compute/README.md`.
- **Artifacts:** `CMakeLists.txt`, `engine/compute/api.hpp`, `engine/compute/src/dispatchers/*.cpp`, `scripts/build/presets/*.json`, `engine/compute/README.md`.
- **Tests:** Build verification with `ENGINE_ENABLE_CUDA=OFF` and `ON`; runtime unit tests ensuring CPU fallback when CUDA is unavailable; CI preset for CPU-only configuration.
- **Docs:** Expand compute module README; update root README with the new build option.

#### DC-003: Platform Backend Abstraction
- **Priority:** MEDIUM
- **Module:** Platform
- **Dependencies:** []
- **Tasks:**
  - [x] Introduce a configurable CMake option `ENGINE_WINDOW_BACKEND` supporting `GLFW`, `SDL`, and `MOCK` values.
  - [x] Implement a factory pattern in `engine/platform/windowing/window_system.cpp` that instantiates the requested backend.
  - [x] Complete the SDL implementation in `engine/platform/src/windowing/sdl_window.cpp` and ensure parity with the GLFW path.
  - [x] Add backend capability queries to `WindowConfig`.
  - [x] Extend the test matrix to cover all backend permutations.
  - [x] Document backend selection and supported features in `docs/modules/platform/README.md`.
- **Artifacts:** `engine/platform/windowing/window_system.cpp`, `engine/platform/src/windowing/sdl_window.cpp`, `engine/platform/include/engine/platform/windowing/window.hpp`, `CMakeLists.txt`, platform module README updates.
- **Tests:** `engine_platform_tests` exercises mock, SDL, and automatic selection paths; CI stays headless-safe via the mock backend.
- **Docs:** Platform module README updates and root build instructions referencing backend selection.

#### DC-004: Error Handling Standardization
- **Priority:** MEDIUM
- **Module:** Cross-cutting
- **Dependencies:** []
- **Tasks:**
  - [x] Define `engine::Result<T, Error>` in `engine/core/diagnostics/result.hpp` and design the supporting type utilities.
  - [x] Create an `engine::ErrorCode` hierarchy in `engine/core/diagnostics/error.hpp` that modules can specialize.
  - [x] Document the error-handling policy in `CODING_STYLE.md` and supporting design docs.
  - [x] Migrate the IO module to the `Result<T>` pattern as a pilot implementation.
  - [x] Provide module templates demonstrating correct usage.
  - [x] Draft a migration guide for existing subsystems.
- **Artifacts:** `engine/core/diagnostics/result.hpp`, `engine/core/diagnostics/error.hpp`, `engine/io` implementation updates, module templates, documentation in `docs/design/error_handling_migration.md`.
- **Tests:** IO module regression suite updated to assert `Result<T>` handling; static analysis or clang-tidy checks for legacy patterns.
- **Docs:** Update coding standards and IO README; provide migration guide references in module docs.

### Architecture Improvements

#### AI-001: Resource Lifetime Management
- **Priority:** HIGH
- **Module:** Rendering, Assets
- **Dependencies:** [DC-004]
- **Tasks:**
  - [x] Design a resource handle system with generation counters.
  - [x] Implement `ResourcePool<T>` in `engine/core/memory/resource_pool.hpp`.
  - [x] Provide `ResourceHandle<T>` wrappers with validation logic in `engine/assets/handles.hpp`.
  - [x] Document ownership patterns in `docs/design/resource_management.md`.
  - [x] Refactor asset caches to use typed handles.
  - [x] Add lifetime validation hooks active in debug builds.
- **Artifacts:** `engine/core/memory/resource_pool.hpp`, `engine/assets/handles.hpp`, asset cache sources, resource management design notes.
- **Tests:** Rendering and asset unit tests covering handle recycling; debug-only assertions verifying stale handle detection.
- **Docs:** Asset and rendering module READMEs updated with the ownership model.

#### AI-002: Async Asset Streaming Architecture
- **Priority:** MEDIUM
- **Module:** Assets, IO, Runtime
- **Dependencies:** [AI-001, DC-001]
- **Tasks:**
  - [x] Author the async loading API design in `docs/design/async_streaming.md`.
  - [x] Implement `AssetLoadRequest` and `AssetLoadFuture` primitives in the Assets module.
  - [x] Provide a background thread pool in Core dedicated to IO workloads.
  - [x] Extend asset caches with a streaming state machine.
  - [x] Introduce a priority queue scheduling load requests.
  - [x] Emit streaming metrics to runtime telemetry.
  - [x] Document best practices in the Assets README.
- **Artifacts:** `docs/design/async_streaming.md`, `engine/assets/include/engine/assets/async.hpp`, `engine/assets/src/streaming/*.cpp`, `engine/core/threading/thread_pool.hpp`, telemetry hooks.
- **Tests:** Asynchronous loading integration tests with deterministic fixtures; runtime telemetry validation.
- **Docs:** Assets README streaming section; runtime telemetry overview.
- **Tracking:** [`T-0115`](tasks/T-0115-assets-async-streaming-mvp.md).

#### AI-003: Frame Graph Resource Metadata
- **Priority:** HIGH
- **Module:** Rendering
- **Dependencies:** []
- **Tasks:**
  - [x] Extend `FrameGraphResourceInfo` with `ResourceFormat`, `ResourceDimension`, `UsageFlags`, `InitialState`, and `FinalState` descriptors.
  - [x] Update `RenderPass` definitions to include queue affinity.
  - [x] Add metadata validation within frame graph compilation.
  - [x] Provide a migration guide for existing pass definitions.
  - [x] Update unit tests in `engine/rendering/tests/test_frame_graph.cpp` to cover new metadata.
  - [ ] Synchronise runtime-submitted pass descriptors with the expanded metadata schema.
  - [ ] Document metadata alignment responsibilities across rendering and runtime READMEs.
- **Artifacts:** `engine/rendering/frame_graph/frame_graph.hpp`, `engine/rendering/frame_graph/render_pass.hpp`, `engine/rendering/tests/test_frame_graph.cpp`, migration guide documentation.
- **Tests:** Frame graph unit tests validating metadata propagation; integration smoke tests.
- **Docs:** Rendering README updates describing metadata and queue affinity.
- **Tracking:** [`T-0104`](tasks/T-0104-runtime-frame-graph-integration.md) (completed).

#### AI-004: Compute Dependency Validation
- **Priority:** MEDIUM
- **Module:** Compute
- **Dependencies:** [DC-002]
- **Tasks:**
  - [ ] Extend the compute dispatcher with dependency metadata.
  - [ ] Detect cyclic dependencies at registration time.
  - [ ] Emit diagnostic graphs for debugging scheduling issues.
  - [ ] Update compute module documentation with dependency modelling guidance.
- **Artifacts:** Compute dispatcher source updates, diagnostic tooling, compute README guidance.
- **Tests:** Unit tests covering cycle detection; regression coverage for dispatcher diagnostics.
- **Docs:** Compute README and tutorials outlining dependency validation workflows.

### Roadmap TODOs

#### RT-001: Animation Deformation Pipeline
- **Priority:** HIGH
- **Module:** Animation, Geometry
- **Dependencies:** [AI-001]
- **Tasks:**
  - [x] Implement rig binding data structures (see [docs/modules/animation/ROADMAP.md](modules/animation/ROADMAP.md)).
  - [x] Integrate linear blend skinning into the runtime evaluation path.
  - [x] Author deformation regression tests using deterministic fixtures.
  - [x] Document deformation workflows in Animation and Geometry READMEs.
- **Artifacts:** Animation deformation sources, geometry binding helpers, documentation updates.
- **Tests:** Runtime smoke tests verifying deformation correctness; geometry unit coverage.
- **Docs:** Animation and geometry module READMEs; deformation migration guides.
- **Tracking:** [`T-0113`](tasks/T-0113-animation-runtime-skinning.md).

#### RT-002: Physics Contact Manifolds
- **Priority:** HIGH
- **Module:** Physics
- **Dependencies:** []
- **Tasks:**
  - [ ] Design persistent manifold data structures.
  - [ ] Integrate constraint solving hooks.
  - [ ] Emit telemetry for collision diagnostics.
  - [ ] Document manifold usage in Physics README.
- **Artifacts:** Physics solver sources, manifold data structures, telemetry hooks, documentation updates.
- **Tests:** Physics regression suite covering manifold stability and constraint solving.
- **Docs:** Physics README, telemetry references.

#### RT-003: Vulkan Backend Prototype
- **Priority:** HIGH
- **Module:** Rendering
- **Dependencies:** [AI-003]
- **Tasks:**
  - [x] Implement a Vulkan backend conforming to the frame graph scheduler (validated through runtime-driven submissions).
  - [x] Provide resource translation layers for Vulkan handles.
  - [x] Author backend configuration documentation and samples.
  - [ ] Keep runtime submission hooks and Vulkan translation layers in parity as metadata evolves.
  - [ ] Document the Rendering/Runtime vertical slice workflow for this backend.
- **Artifacts:** Vulkan backend sources, CMake targets, documentation.
- **Tests:** Rendering backend smoke tests; CI presets exercising Vulkan.
- **Docs:** Rendering README backend section; integration notes in root README.
- **Tracking:** [`T-0104`](tasks/T-0104-runtime-frame-graph-integration.md) (scheduler prototype), [`T-0116`](tasks/T-0116-rendering-vulkan-resource-translation.md).

#### RT-004: Runtime Diagnostics
- **Priority:** HIGH
- **Module:** Runtime, Core
- **Dependencies:** []
- **Tasks:**
  - [ ] Instrument runtime lifecycle stages with telemetry hooks.
  - [ ] Expose diagnostics through `RuntimeHost` APIs.
  - [ ] Surface diagnostics in tooling dashboards.
- **Artifacts:** Runtime diagnostics sources, telemetry exporters, tooling integration.
- **Tests:** Runtime unit tests covering diagnostics; tooling integration tests.
- **Docs:** Runtime README diagnostics section; tooling documentation.

#### RT-005: Scene Hierarchy Validation
- **Priority:** HIGH
- **Module:** Scene
- **Dependencies:** []
- **Tasks:**
  - [ ] Implement hierarchy validation utilities guarding against cycles and invalid transforms.
  - [ ] Add reporting hooks to surface issues during runtime synchronization.
  - [ ] Document validation workflows in Scene README.
- **Artifacts:** Scene validation sources, diagnostics, documentation.
- **Tests:** Scene regression suite covering hierarchy manipulation; runtime integration tests.
- **Docs:** Scene README updates; runtime troubleshooting notes.

#### RT-006: IO Format Detection Hardening
- **Priority:** HIGH
- **Module:** IO
- **Dependencies:** []
- **Tasks:**
  - [ ] Expand signature databases for mesh, animation, and point-cloud formats.
  - [ ] Integrate fuzzing harnesses for parser robustness.
  - [x] Provide structured error reporting (landed via [docs/modules/io/ROADMAP.md](modules/io/ROADMAP.md)).
- **Artifacts:** IO detection sources, fuzzing harnesses, documentation.
- **Tests:** IO regression suite; fuzzing automation.
- **Docs:** IO README detailing detection heuristics and failure modes.
- **Tracking:** [`T-0112`](tasks/T-0112-geometry-io-roundtrip-hardening.md).

### Documentation Improvements

#### DI-001: README Standardisation
- **Priority:** MEDIUM
- **Module:** Documentation
- **Dependencies:** []
- **Tasks:**
  - [ ] Ensure every module README follows the shared template.
  - [ ] Cross-reference module TODOs with the central roadmap.
  - [ ] Automate linting for README completeness via `scripts/validate_docs.py`.
- **Artifacts:** Updated README files, validation scripts.
- **Tests:** Documentation validation in CI.
- **Docs:** Guidance within `docs/README_TEMPLATE.md`.

#### DI-002: API Reference Generation
- **Priority:** MEDIUM
- **Module:** Documentation
- **Dependencies:** []
- **Tasks:**
  - [ ] Automate API reference extraction for public headers.
  - [ ] Publish the output under `docs/api/`.
  - [ ] Document the generation workflow.
- **Artifacts:** API generation scripts, published documentation.
- **Tests:** Documentation validation ensuring links remain valid.
- **Docs:** Root README build documentation.

#### DI-003: Architecture Decision Records
- **Priority:** LOW
- **Module:** Documentation
- **Dependencies:** []
- **Tasks:**
  - [ ] Establish an ADR format under `docs/decisions/`.
  - [ ] Backfill key historical decisions.
  - [ ] Reference ADRs from module READMEs.
- **Artifacts:** ADR markdown files, README updates.
- **Tests:** Documentation validation.
- **Docs:** Documentation README instructions.

### Build System Enhancements

#### BS-001: Preset Expansion
- **Priority:** MEDIUM
- **Module:** Build System
- **Dependencies:** []
- **Tasks:**
  - [ ] Add build presets covering common feature-flag combinations.
  - [ ] Document preset usage in the root README.
  - [ ] Validate presets via CI.
- **Artifacts:** `CMakePresets.json`, `scripts/build/presets/*.json`, documentation updates.
- **Tests:** CI runs for each preset; local build validation.
- **Docs:** Root README build workflow section.

#### BS-002: Versioning Policy
- **Priority:** MEDIUM
- **Module:** Build System
- **Dependencies:** []
- **Tasks:**
  - [ ] Document minimum compiler/CMake versions.
  - [ ] Establish vendored dependency update cadence.
  - [ ] Integrate checks into CI to enforce versions.
- **Artifacts:** Documentation updates, CI checks.
- **Tests:** CI verification of version requirements.
- **Docs:** Root README and tooling notes.

#### BS-003: CMake Target Hygiene
- **Priority:** LOW
- **Module:** Build System
- **Dependencies:** []
- **Tasks:**
  - [ ] Audit CMake targets for unnecessary dependencies.
  - [ ] Apply `PRIVATE`/`PUBLIC` scoping consistently.
  - [ ] Document conventions in `CODING_STYLE.md` and build docs.
- **Artifacts:** CMake updates, documentation.
- **Tests:** Build verification across presets.
- **Docs:** Build documentation updates.

### Testing Infrastructure

#### TI-001: Integration Suites
- **Priority:** HIGH
- **Module:** Testing
- **Dependencies:** []
- **Tasks:**
  - [x] Stand up deterministic integration tests across animation, physics, runtime, and rendering.
  - [x] Provide fixtures and harnesses under `engine/tests/integration/`.
  - [x] Document integration test guidelines.
- **Artifacts:** Integration test sources, fixtures, documentation.
- **Tests:** Integration test executions across configurations.
- **Docs:** `engine/tests/integration/README.md` and testing overview updates.
- **Tracking:** [`T-0114`](tasks/T-0114-testing-integration-suites.md).

#### TI-002: Performance Benchmarks
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

#### TI-003: Fuzzing Harnesses
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

### Python Bindings Completeness

#### PY-001: Core Bindings
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

### Cross-Cutting Initiatives

#### CC-001: Telemetry Framework
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

#### CC-002: Hot Reload Infrastructure
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

### Milestone Coordination

#### MC-001: Milestone Dashboard
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

#### MC-002: Synchronize Module Roadmaps
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

### Priority Summary

| Horizon | Rank | Item | Status |
|---------|------|------|--------|
| Sprint 1 Focus | 1 | AI-003 | In Progress — align frame-graph metadata with runtime submissions. |
|                 | 2 | RT-001 | Complete — keep deformation regression coverage healthy. |
|                 | 3 | TI-001 | Complete — integration suites act as the baseline smoke tests. |
| Sprint 2-3 | 1 | DC-004 | In Progress — finish the error-handling migration and docs. |
|            | 2 | AI-001 | In Progress — extend handle-based lifetime management. |
|            | 3 | RT-002 | Planned — deliver persistent manifolds plus diagnostics. |
|            | 4 | RT-004 | Planned — surface runtime lifecycle telemetry through tooling. |
|            | 5 | DI-001 | Planned — enforce README standardisation and automation. |
| Mid-Term (M4-M5) | 1 | AI-002 | Planned — stand up async asset streaming with telemetry. |
|                   | 2 | RT-003 | In Progress — maintain Vulkan backend parity with runtime. |
|                   | 3 | CC-001 | Planned — design and integrate the telemetry framework. |
|                   | 4 | PY-001 | Planned — expose bindings with pytest coverage. |

This plan should be revisited at the start of each milestone to adjust priorities, confirm dependencies, and align with evolving product goals.

## Module Roadmaps

- [Animation](modules/animation/ROADMAP.md)
- [Assets](modules/assets/ROADMAP.md)
- [Compute](modules/compute/ROADMAP.md)
- [Core](modules/core/ROADMAP.md)
- [Geometry](modules/geometry/ROADMAP.md)
- [IO](modules/io/ROADMAP.md)
- [Math](modules/math/ROADMAP.md)
- [Physics](modules/physics/ROADMAP.md)
- [Platform](modules/platform/ROADMAP.md)
- [Rendering](modules/rendering/ROADMAP.md)
- [Runtime](modules/runtime/ROADMAP.md)
- [Scene](modules/scene/ROADMAP.md)
- [Tools](modules/tools/ROADMAP.md)

## Milestone Bands

- **Near-Term (1–2 milestones)** – Close the most painful feature gaps to unblock multi-module integration and smoke-test scenarios.
- **Mid-Term (3–5 milestones)** – Broaden functionality and harden subsystems for sustained iteration, including cross-module data flows and tooling.
- **Long-Term (5+ milestones)** – Deliver robustness, authoring experiences, and extensibility required for production workloads.

## Milestone Spotlight – First Rendering Frame

### Objective

Deliver the first on-screen frame produced by the engine by aligning the rendering and runtime roadmaps
around a tightly scoped integration slice. The milestone emphasises the minimum vertical path capable of
submitting a renderable scene graph from the runtime into the rendering frame graph and replaying it through
a backend-neutral scheduler.

### Selected TODOs

1. **Enrich Frame-Graph Resource Descriptors**
   - **Source backlog:** `engine/rendering/README.md` short-term roadmap.
   - **Description:** Extend `FrameGraphResourceInfo` and related creation APIs with explicit format,
     dimension, and usage metadata so backend resource providers can allocate deterministically and passes
     publish the constraints required for validation.
   - **Deliverables:** Schema updates across the rendering resources, a migration guide documenting the new
     fields and defaults, and unit coverage asserting the metadata appears in compiled graphs.
   - **Dependencies:** Coordinate schema alignment with the assets module so material and shader descriptors
     carry matching requirements.

2. **Prototype the Reference GPU Scheduler**
   - **Source backlog:** Rendering roadmap short-term goals.
   - **Description:** Implement a backend-neutral scheduler that converts compiled frame-graph passes into a
     linear submission stream targeting an abstract command encoder. Validate dependency resolution,
     transient lifetime management, and queue metadata propagation before platform backends exist.
   - **Deliverables:** Reference scheduler implementation with stub encoder hooks and logging, integration
     tests covering multi-pass graphs, and diagnostics surfaced through the logging infrastructure to
     visualise submission order.
   - **Dependencies:** Builds directly on the enriched resource descriptors and exposes the data needed by
     runtime scheduling hooks.

3. **Introduce Runtime Render Submission Hooks**
   - **Source backlog:** Runtime roadmap mid-term goals.
   - **Description:** Extend `RuntimeHost` with a render submission interface (e.g., `RenderGraphBuilder`)
     that packages the current scene snapshot, camera parameters, and resource requirements into a form the
     rendering scheduler consumes. The runtime owns orchestration of visibility queries and pass registration
     for the initial slice.
   - **Deliverables:** Runtime façade or adapter that marshals scene data into renderable structures, a smoke
     test exercising animation → physics → runtime submission → rendering scheduler, and documentation that
     illustrates the lifecycle from runtime tick to render submission.
   - **Dependencies:** Requires TODOs 1 and 2 to define the data contracts accepted by the rendering
     scheduler.

### Acceptance Criteria

- A single example application can tick the runtime, build a frame graph, and submit it through the
  reference GPU scheduler without backend-specific code.
- CI includes regression coverage for resource descriptor enrichment, scheduler submission ordering, and
  runtime-to-rendering handoff.
- Documentation across runtime and rendering READMEs references this milestone to orient future
  contributors.

## Subsystem Alignment

### Animation

- **Completed (M1–M2):**
  - ✅ Clip validation, JSON serialization
  - ✅ Linear blend trees with parameter binding
  - ✅ Float, bool, and event parameters
- **Near-Term (M3):**
  - Additive blend nodes for pose composition
  - Deformation binding data structures
- **Mid-Term (M4–M5):**
  - Linear and dual quaternion skinning
  - State machine nodes
  - Editor authoring tools

### Assets

- **Near-Term** – Formalise asset descriptors, improve hot-reload diagnostics, and align cache metadata with IO importers so pipelines can reason about provenance.
- **Mid-Term** – Stand up staged import graphs that feed runtime caches and persist cooked artefacts for reuse across runs.
- **Long-Term** – Automate validation with representative sample sets and integrate cache residency policies into tooling workflows.

### Compute

- **Near-Term** – Unify dispatch descriptions so CPU and GPU executors can share the kernel scheduler and timing instrumentation.
- **Mid-Term** – Introduce the CUDA-backed executor, surface device/stream management hooks, and expose mixed CPU/GPU regression scenarios.
- **Long-Term** – Integrate with the runtime job system and extend profiling/telemetry so heterogeneous workloads remain observable.

### Core

- **Near-Term** – Define the application lifecycle, configuration, and diagnostics services that the runtime will host, leveraging the existing EnTT façade.
- **Mid-Term** – Implement plugin discovery/loading with deterministic teardown and expand configuration layering (defaults, file overrides, command line).
- **Long-Term** – Harden coverage and author examples that demonstrate how downstream modules consume the shared services.

### Geometry & IO

- **Near-Term** – Complete the foundational read/write paths for meshes, point clouds, and graphs while keeping property registries and spatial structures aligned with runtime needs.
- **Mid-Term** – Add remeshing, parameterisation, and reconstruction algorithms that feed deformation and collision pipelines, alongside richer detection diagnostics.
- **Long-Term** – Institutionalise benchmarking, profiling, and documentation to maintain data quality as advanced algorithms land.

### Math

- **Near-Term** – Catalogue the current primitives, document invariants, and broaden unit tests around existing helpers (reflection, inversion, decomposition).
- **Mid-Term** – Introduce fixed-size factorisation routines (QR/SVD/Cholesky) and SIMD specialisations required by animation/physics.
- **Long-Term** – Expand interoperability layers with geometry/physics and expose conversion utilities to the Python bindings.

### Physics

- **Near-Term** – The rigid-body world exposes damping, substepping, sweep-and-prune broad-phase pruning, and capsule/sphere/AABB colliders. The priority now is contact manifold generation, constraint solving, and instrumentation as captured in [modules/physics/ROADMAP.md](modules/physics/ROADMAP.md).
- **Mid-Term** – Scale collision management with persistent manifolds and constraint solvers so physics can drive richer runtime scenes.
- **Long-Term** – Advance dynamics fidelity with improved integrators, sleeping/activation heuristics, and an extensible collider set coordinated with geometry.

### Rendering

- **Near-Term** – Extend frame-graph resource descriptors, propagate queue/command metadata, and prototype a reference GPU scheduler according to [modules/rendering/ROADMAP.md](modules/rendering/ROADMAP.md).
- **Mid-Term** – Provide backend resource providers (Vulkan/DX12) and schedulers along with a baseline library of passes.
- **Long-Term** – Layer on validation, profiling, and extensive documentation/samples to support production use.

### Runtime

- **Near-Term** – `RuntimeHost` already orchestrates animation → physics → geometry via the compute dispatcher. Harden lifecycle diagnostics, scene mirroring, and streaming hooks as detailed in the [Runtime Expansion Plan](#runtime-expansion-plan).
- **Mid-Term** – Integrate asynchronous asset streaming and render submission so the runtime can coordinate full-frame workloads.
- **Long-Term** – Move onto the global job system, support deterministic replay, and expose hot-reloadable configuration for live tuning.

#### Runtime Expansion Plan

- **Current Observations**
  - `RuntimeHost` replaces the ad-hoc singleton while advancing animation, physics, geometry, and scene mirroring through the fixed kernel chain (`animation.evaluate → physics.accumulate → physics.integrate → geometry.deform → geometry.finalize`).
  - Initialization builds a toy physics world, generates a linear animation clip, and mirrors joint transforms into an EnTT-backed scene registry accessible through C++ and C front-ends.
  - Shutdown resets transient buffers but lacks subsystem teardown, diagnostics, streaming, or render scheduling.

- **Near-Term Goals (1–2 Milestones)**
  1. **Formalise lifecycle management.** Replace implicit singletons with explicit ownership, extend lifecycle diagnostics, and expand tests that exercise repeated initialize/shutdown sequences and invalid usage.
  2. **Extend frame orchestration.** Promote the dispatcher from a fixed linear chain to a frame-graph-driven scheduler, surface per-kernel timing through `runtime_frame_state`, and add tests validating topological execution and telemetry integrity.
  3. **Scene synchronisation hardening.** Support dynamic entity lifecycles when content streams in, preserve hierarchy metadata, and add regressions that verify world transforms for branching rigs.

- **Mid-Term Goals (3–5 Milestones)**
  4. **Streaming asset integration.** Interface with assets/IO for asynchronous loading, maintain double-buffered resources, and test resilience to slow streams.
  5. **Render scheduling hooks.** Define a render submission interface that packages scene snapshots and propagates bounds/camera configuration, then validate an end-to-end smoke test.
  6. **Diagnostics and tooling.** Emit structured telemetry, integrate profiling hooks, and expand runtime documentation with lifecycle diagrams and troubleshooting guidance.

- **Long-Term Goals (5+ Milestones)**
  7. **High-frequency job system integration.** Migrate the dispatcher onto the global task graph, track CPU/GPU dependencies, and gate resource usage with completion fences.
  8. **Deterministic replay and state capture.** Record authoritative simulation inputs each frame, persist snapshots, and feed tooling pipelines for offline analysis.
  9. **Hot-reloadable configuration.** Expose configuration files or scripting bindings for runtime parameters, backed by validation layers that reject incompatible changes.

- **Documentation and Tracking**
  - Update the runtime README as milestones close and mirror progress into the central roadmap and CI/backlog dashboards so dependent teams remain aligned.

### Scene

- **Near-Term** – Define schemas for first-class runtime components (lights, cameras, visibility volumes) and extend traversal helpers beyond raw registry views.
- **Mid-Term** – Broaden serialization to cover the new component families, add versioning, and ensure forward/backward compatibility tests land alongside loaders.
- **Long-Term** – Maintain profiling scenarios and authoring tooling that stress lifecycle, hierarchy manipulation, and serialization throughput.

### Platform

- **Near-Term** – Replace mock window/input providers with real GLFW/SDL integrations and extend the virtual filesystem with write/watch utilities to unblock hot reload.
- **Mid-Term** – Harden backend selection, expose advanced window features (high DPI, fullscreen, swapchain surfaces), and wire real input sampling into the runtime.
- **Long-Term** – Deliver robust diagnostics, configuration toggles, and automated coverage for cross-platform deployments.

### Tools

- **Near-Term** – Stand up shared tooling infrastructure (common utilities, Dear ImGui backends) and prototype the asset pipeline CLI.
- **Mid-Term** – Integrate profiling data capture and live viewers, then iterate on editor shells with dockable panels and hot-reload hooks.
- **Long-Term** – Package distribution-ready tooling with documentation, telemetry options, and reproducible sample projects.

## Cross-Cutting Initiatives

1. **Data Contracts** – Animation, physics, and geometry must converge on shared mesh, rig, and collider representations before runtime orchestration expands. Geometry's foundational work is a prerequisite for advanced animation deformation and physics manifolds.
2. **Scheduling Infrastructure** – Rendering's scheduler prototype and compute's unified dispatch abstractions inform the runtime's migration to a job-graph-driven frame loop.
3. **Asset Pipelines** – The IO and asset modules must deliver reliable import/cache flows before tooling and runtime streaming can stabilise.
4. **Diagnostics** – Profiling hooks introduced in physics, rendering, and runtime feed the tooling roadmap and should share consistent telemetry schemas.
