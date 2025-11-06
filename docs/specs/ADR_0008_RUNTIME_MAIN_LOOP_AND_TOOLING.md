# ADR-0008: Runtime Main Loop and Tooling Integration

- **Status:** Proposed
- **Drivers:** `RT-320`, `TL-210`, `RE-610`
- **Authors:** Chief Architect Working Group
- **Last Updated:** 2026-02-15

## Context

The runtime currently exposes `RuntimeHost::tick` as a monolithic orchestrator that advances
animation, physics, geometry deformation, streaming, scene validation, and rendering
submission. Diagnostics cover each stage, yet the loop has limited configurability beyond
enabling or disabling entire subsystems. Tooling (ImGui diagnostics, experiment sandbox,
telemetry viewers) integrates directly with runtime internals instead of sharing a cohesive
extension surface. Rendering backends (OpenGL, Vulkan prototype, future DirectX 12) are
wired via the frame-graph scheduler but lack a formal contract for swap-chain/presentation
ownership and UI overlay composition.

Upcoming milestones highlight the gaps:

- `RT-320` needs a schema-driven harness capable of running in headless and interactive modes
  with deterministic scheduling and instrumentation hooks.
- `TL-210` requires the sandbox UI to reuse diagnostics panels and render hierarchy/component
  inspectors on demand without duplicating layout code.
- `RE-610` mandates backend parity for the research rendering baseline, including
  consistent presentation paths and overlay rendering across backends.

Without a structured main loop abstraction and shared UI tooling surface, each initiative will
clone glue code, jeopardise determinism, and complicate backend validation.

## Decision

1. **Introduce a declarative runtime loop description.**
  - Define `RuntimeLoopStage` as a lightweight descriptor: name, update function, execution
    phase (Simulation, Presentation, Diagnostics), thread affinity hint (enumerated as
    `RuntimeLoopThreadAffinity::{MainThread, WorkerThread, Any}`), and dependencies.
   - Provide `RuntimeLoopBuilder` to assemble a directed acyclic graph of stages. It defaults to
     the canonical ordering (`Input → Animation → Physics → Geometry → Scene → Rendering → UI →
     Diagnostics → Present`) while permitting overrides (insertion, replacement, removal) that
     preserve dependency constraints.
   - `RuntimeHost` owns a compiled `RuntimeLoopPlan` produced by the builder at initialization.
     Each `tick` iterates the plan, executing stages with deterministic ordering and emitting
     per-stage telemetry.
   - Expose planner contracts so tooling and backends can interact with the compiled plan without
     duplicating scheduler knowledge. The API surface comprises:

     ```cpp
     struct StageBudget {
       double target_ms;                  // Nominal budget used for telemetry comparisons.
       bool enforce_budget;               // Emit diagnostics when the budget is exceeded.
     };

     struct StageHandle {
       StageId id;                        // Stable identifier generated from stage name.
       std::string_view name;             // Human-readable label used by diagnostics/tooling.
       RuntimeLoopPhase phase;            // Simulation, Presentation, Diagnostics.
       RuntimeLoopThreadAffinity affinity;// Scheduling hint enforced by the planner.
       StageBudget budget;                // Presentation + telemetry budget metadata.
       Span<const StageId> dependencies;  // Topologically sorted upstream requirements.
     };

     class RuntimeStagePlanner {
     public:
       void Reset(const RuntimeLoopPlan& plan);
       Expected<StageExecution, PlannerError> AcquireNextStage(RuntimeContext& ctx);
       void CompleteStage(StageExecutionToken token, StageResult result);
       const RuntimeLoopPlan& InspectPlan() const noexcept;
     };
     ```

     - `StageExecution` encapsulates a `StageHandle`, execution state, and timing accumulators.
     - `StageExecutionToken` is an opaque handle guaranteeing balanced `Acquire`/`Complete` calls
       for profiling and synchronization.
     - `PlannerError` enumerates structural faults (cycle detection, missing dependencies) and
       runtime errors (stage execution failures, timeout budget overruns).

2. **Separate backend submission from presentation.**
   - Extend the rendering contract with a `PresentationBackend` interface encapsulating swap
     chain acquisition, Dear ImGui draw data submission, and headless readback hooks.
   - Frame-graph execution populates render targets and returns a `FrameOutput` handle. The loop
     passes this handle to the presentation backend, which resolves the final image, composites
     UI overlays, and performs presentation or readback.
   - Standardise the interface so runtime/tooling code can manage lifecycle transitions:

     ```cpp
     class PresentationBackend {
     public:
       virtual ~PresentationBackend() = default;

       virtual Expected<void, PresentationError>
       Initialize(const PresentationConfig& config) = 0;   // Acquire swapchain/window state.

       virtual Expected<PresentationFrame, PresentationError>
       BeginFrame(const StageHandle& present_stage) = 0;    // Import runtime frame outputs.

       virtual Expected<void, PresentationError>
       CompositeOverlays(PresentationFrame& frame,
                         const tools::imgui::DrawData* overlays) = 0; // Optional.

       virtual Expected<void, PresentationError>
       Submit(PresentationFrame&& frame) = 0;               // Present or perform readback.

       virtual void Shutdown() noexcept = 0;                // Release native resources.
     };
     ```

     - `PresentationConfig` captures windowing handles, vsync policy, colour space, and headless
       readback toggles negotiated with platform services.
     - `PresentationFrame` owns the resolved render targets and any synchronization fences needed
       to guarantee GPU/CPU ownership transfer.
     - `PresentationError` codes align with runtime error handling (`engine/runtime/errors.hpp`).
   - Provide built-in implementations for OpenGL (existing queue-normalised path) and a Vulkan
     WSI bridge. Mock/headless mode exposes a CPU readback presenter for CI.

3. **Standardise ImGui panel reuse through registry-based composition.**
   - Create `tools::imgui::PanelRegistry`, allowing modules to register panels via stable IDs and
     render callbacks (`void(RenderContext&)`).
   - Runtime and sandbox code request panels by ID, enabling reuse of hierarchy inspectors,
     telemetry panels, and material browsers without duplicating layout logic.
   - Panels declare required diagnostics handles or asset references so registrants can validate
     availability before rendering.

4. **Expose instrumentation and scripting hooks.**
   - Each `RuntimeLoopStage` automatically creates a Tracy zone and emits structured telemetry.
   - Add a scripting bridge (Python + C API) to configure the loop plan (enable/disable stages,
     inject callbacks) for the prototyping harness. Changes trigger recompilation of the plan at
     safe synchronization points (before the next frame begins).
   - Provide a `RuntimeLoopInspector` utility that dumps the compiled stage graph (topology,
     timings, dependencies) for tooling consumers.
   - Export synchronization helpers through the C interface so lightweight tooling can observe
     presentation state without embedding C++. `engine_runtime_presentation_stage_active()` reports
     whether the presentation stage is currently wired, while
     `engine_runtime_loop_plan_serialization()` returns the JSON-encoded stage sequence used by the
     active runtime host. Python bindings in `engine3g.Loader` surface these helpers via
     `EngineRuntimeHandle.presentation_stage_active()` and `.loop_plan_serialization()` so hybrid
     harnesses can gate UI capture or confirm scheduler topology directly from scripts.

5. **Document integration contracts.**
   - Record invariants in module READMEs and update the runtime/tooling APIs to require explicit
     registration of stages/panels. Add samples demonstrating sandbox consumption of reusable
     panels and headless presentation with ImGui disabled.

## Consequences

- **Positive**
  - Deterministic, configurable main loop supporting both headless harness execution and
    interactive sandbox workflows without branching logic.
  - Rendering backends share presentation plumbing, enabling consistent UI compositing and
    screenshot capture across OpenGL/Vulkan/Mock.
  - Tooling teams render diagnostics panels by ID, reducing duplicated ImGui layout code and
    improving testability.
  - Telemetry and scripting hooks simplify performance validation and automated experimentation.

- **Negative**
  - Requires refactoring `RuntimeHost` construction to build the loop plan, touching integration
    tests and diagnostics snapshots.
  - Presentation backend abstraction introduces a layer of indirection that backends must
    implement before new features ship.
  - Panel registry demands coordination between tooling and runtime teams to define stable IDs
    and dependency schemas.

## Implementation Plan

1. Author header/API updates:
   - `engine/runtime/include/.../loop.hpp` for `RuntimeLoopStage`, `RuntimeLoopBuilder`, and
     inspection utilities.
   - `engine/runtime/include/.../stage_planner.hpp` for `StageHandle`, `RuntimeStagePlanner`, and
     associated error/result types consumed by tooling and scripting.
   - `engine/rendering/include/.../presentation_backend.hpp` describing the new interface and
     default implementations, plus shared config/descriptor types consumed by runtime/tests.
   - `engine/tools/include/engine/tools/imgui/panel_registry.hpp` plus supporting source files.

2. Refactor `RuntimeHost` initialization to build the default loop plan and expose configuration
   hooks (`RuntimeLoopConfig`). Update diagnostics to serialise the plan for tooling.

3. Integrate presentation backends:
   - Wrap existing OpenGL submission path.
   - Finalise Vulkan presenter; create mock/headless presenter returning CPU-readable buffers.
   - Adapt runtime/tests to validate presenter selection and ImGui compositing toggles.

4. Port existing ImGui panels (scene hierarchy, streaming metrics, telemetry overlays) to the
   registry and update sandbox/runtime UI code to fetch panels by ID.

5. Extend Python harness bindings to configure the loop and request screenshots/readbacks through
   the presentation backend.

6. Update documentation and samples:
   - Module READMEs (runtime, rendering, tools) referencing this ADR and demonstrating usage.
   - Sandbox README illustrating panel reuse and headless rendering toggles.

## Open Questions

- Should loop stages support asynchronous execution groups (e.g., running animation/physics in
  parallel)? Proposal: defer to a follow-up ADR once deterministic single-threaded ordering is
  battle-tested and telemetry baselines exist.
- How should persistent UI state (e.g., window docking layouts) integrate with the panel registry?
  Proposal: allow panels to expose optional persistence blocks handled by the sandbox/runtime host.
- Do we expose presentation backends directly to scripting for screenshot capture, or require
  loop-stage adapters? Decision deferred until harness scripting requirements stabilise.

## References

- [`docs/modules/runtime/README.md`](../modules/runtime/README.md)
- [`docs/modules/rendering/README.md`](../modules/rendering/README.md)
- [`docs/modules/tools/README.md`](../modules/tools/README.md)
- [`docs/specs/ADR_0003_RUNTIME_FRAME_GRAPH.md`](ADR_0003_RUNTIME_FRAME_GRAPH.md)
