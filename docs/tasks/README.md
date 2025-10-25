# Task Records

Each file in this directory documents a sprint backlog or a focused piece of work. Tasks drive discussions with ChatGPT and pair programmers—link to them from PRs and commit messages whenever possible. Treat them as the actionable layer beneath the architecture improvement plan summarised in [`../ROADMAP.md`](../ROADMAP.md).

## How to Use Task Records

1. **Find the relevant file** before starting work. Sprint summaries follow the naming pattern `YYYY-MM-DD-sprint-XX.md`. Individual tickets use `T-####-short-title.md`.
2. **Verify acceptance criteria.** If something is unclear, add clarifying bullets before touching code.
3. **Update the checklist** as you complete deliverables. Keep benchmarks and metrics in the task file for future reference, and bubble materialised learnings back into the roadmap when they impact future priorities.
4. **Link supporting specs** (ADR, RFP) so reviewers can trace intent. Add cross-references from [`ARCHITECTURE.md`](../ARCHITECTURE.md) when a decision introduces a new invariant.

## Current Focus

- **`AN-230` — GPU Parallel Sampling Benchmarks**
  - **Why now?** With [`CO-170`](../ROADMAP.md) complete, the animation and compute teams can execute the benchmark plan in [`docs/design/ANIMATION_GPU_PARALLEL_SAMPLING_BENCHMARK.md`](../design/ANIMATION_GPU_PARALLEL_SAMPLING_BENCHMARK.md) to quantify GPU queue performance and unblock downstream animation work.
  - **Value**: Produces reproducible CPU/GPU comparisons, validates dispatcher queue budgets under animation workloads, and seeds telemetry dashboards for future optimisation.
  - **Next Step**: Automate GPU vs CPU report generation and surface dispatcher telemetry dashboards as scoped in [`CO-170-runtime-integration-sample.md`](CO-170-runtime-integration-sample.md).

## Index

- [`2025-02-17-sprint-06.md`](2025-02-17-sprint-06.md) – current sprint alignment.
- [`T-0104-runtime-frame-graph-integration.md`](T-0104-runtime-frame-graph-integration.md) – runtime/rendering bridge work.
- [`T-0112-geometry-io-roundtrip-hardening.md`](T-0112-geometry-io-roundtrip-hardening.md) – geometry/IO fidelity tasks.
- [`T-0113-animation-runtime-skinning.md`](T-0113-animation-runtime-skinning.md) – deliver the remaining `RT-001` deformation pipeline work.
- [`T-0114-testing-integration-suites.md`](T-0114-testing-integration-suites.md) – build the cross-module integration test harness for `TI-001`.
- [`T-0118-testing-framework-upgrade.md`](T-0118-testing-framework-upgrade.md) – restore fixture support in the Googletest dependency to unblock `TI-001`.
- [`DC-003-sdl-backend-implementation.md`](DC-003-sdl-backend-implementation.md) – execute the SDL backend parity plan for `DC-003`.
- [`DC-003.1-sdl-window-lifecycle.md`](DC-003.1-sdl-window-lifecycle.md) – implement native SDL window lifecycle, deterministic event pumping, and telemetry to unblock `DC-003`.
- [`DC-003.2-sdl-swapchain-surface-export.md`](DC-003.2-sdl-swapchain-surface-export.md) – deliver SDL swapchain surface export and fallback alignment so rendering/tests can execute without GLFW.
- [`DC-003.3-sdl-ci-telemetry.md`](DC-003.3-sdl-ci-telemetry.md) – expand SDL CI coverage and telemetry instrumentation to close roadmap initiative `DC-003`.
- [`CO-170-runtime-integration-sample.md`](CO-170-runtime-integration-sample.md) – activate the runtime ↔ compute sample, telemetry workflow, and documentation updates that unblock GPU sampling benchmarks.
- [`T-0115-assets-async-streaming-mvp.md`](T-0115-assets-async-streaming-mvp.md) – complete the async streaming MVP for `AI-002`.
- [`T-0116-rendering-vulkan-resource-translation.md`](T-0116-rendering-vulkan-resource-translation.md) – finish Vulkan backend deliverables for `RT-003`.
- [`T-0119-rendering-command-encoder-implementation.md`](T-0119-rendering-command-encoder-implementation.md) – implement concrete command encoders for all graphics backends.
- [`T-0120-rendering-gpu-resource-provider-implementation.md`](T-0120-rendering-gpu-resource-provider-implementation.md) – implement GPU resource creation and management for all backends.
- [`T-0121-rendering-standard-passes-library.md`](T-0121-rendering-standard-passes-library.md) – build comprehensive library of standard render passes (shadows, lighting, post-processing).
- [`T-0122-rendering-visibility-culling-system.md`](T-0122-rendering-visibility-culling-system.md) – implement frustum culling, occlusion culling, and LOD systems.
- [`T-0123-rendering-pipeline-state-management.md`](T-0123-rendering-pipeline-state-management.md) – implement PSO caching, shader management, and state tracking.
- [`T-0124-rendering-lighting-system.md`](T-0124-rendering-lighting-system.md) – implement comprehensive lighting with PBR shading, shadows, and light culling.

Create new task files as work is planned. Archive completed tasks under a `done/` subdirectory if they remain valuable references.
