# Task Records

Each file in this directory documents a sprint backlog or a focused piece of work. Tasks drive discussions with ChatGPT and pair programmers—link to them from PRs and commit messages whenever possible. Treat them as the actionable layer beneath the architecture improvement plan summarised in [`../../../../ROADMAP.md`](../../../../ROADMAP.md).

## How to Use Task Records

1. **Find the relevant file** before starting work. Sprint summaries follow the naming pattern `YYYY-MM-DD-sprint-XX.md`. Individual tickets use `T-####-short-title.md`.
2. **Verify acceptance criteria.** If something is unclear, add clarifying bullets before touching code.
3. **Update the checklist** as you complete deliverables. Keep benchmarks and metrics in the task file for future reference, and bubble materialised learnings back into the roadmap when they impact future priorities.
4. **Link supporting specs** (ADR, RFP) so reviewers can trace intent. Add cross-references from [`ARCHITECTURE.md`](../../../../ARCHITECTURE.md) when a decision introduces a new invariant.

## Current Focus

- **`AI-004` — Application Prototyping Enablement** (🚀 Active)
  - **Intent**: Provide a turnkey research workflow by aligning rendering baseline (`RE-610`), runtime prototyping harness (`RT-320`), experiment sandbox UI (`TL-210`), reference dataset packages (`AS-330`), and comparative benchmark automation (`CC-310`).
  - **Next Milestone**: Kickoff review (2025-12-05) confirming shared configuration schema and dataset shortlist; first integration demo targeted for 2025-12-23.

## Index

- [`2025-02-17-SPRINT_06.md`](2025-02-17-SPRINT_06.md) – current sprint alignment.
- [`T_0104_RUNTIME_FRAME_GRAPH_INTEGRATION.md`](T_0104_RUNTIME_FRAME_GRAPH_INTEGRATION.md) – runtime/rendering bridge work.
- [`T_0112_GEOMETRY_IO_ROUNDTRIP_HARDENING.md`](T_0112_GEOMETRY_IO_ROUNDTRIP_HARDENING.md) – geometry/IO fidelity tasks.
- [`T_0113_ANIMATION_RUNTIME_SKINNING.md`](T_0113_ANIMATION_RUNTIME_SKINNING.md) – deliver the remaining `RT-001` deformation pipeline work.
- [`T_0114_TESTING_INTEGRATION_SUITES.md`](T_0114_TESTING_INTEGRATION_SUITES.md) – build the cross-module integration test harness for `TI-001`.
- [`T_0118_TESTING_FRAMEWORK_UPGRADE.md`](T_0118_TESTING_FRAMEWORK_UPGRADE.md) – restore fixture support in the Googletest dependency to unblock `TI-001`.
- [`CO_170_RUNTIME_INTEGRATION_SAMPLE.md`](CO_170_RUNTIME_INTEGRATION_SAMPLE.md) – activate the runtime ↔ compute sample, telemetry workflow, and documentation updates that unblock GPU sampling benchmarks.
- [`T_0115_ASSETS_ASYNC_STREAMING_MVP.md`](T_0115_ASSETS_ASYNC_STREAMING_MVP.md) – complete the async streaming MVP for `AI-002`.
- [`T_0116_RENDERING_VULKAN_RESOURCE_TRANSLATION.md`](T_0116_RENDERING_VULKAN_RESOURCE_TRANSLATION.md) – finish Vulkan backend deliverables for `RT-003`.
- [`T_0119_RENDERING_COMMAND_ENCODER_IMPLEMENTATION.md`](T_0119_RENDERING_COMMAND_ENCODER_IMPLEMENTATION.md) – implement concrete command encoders for all graphics backends.
- [`T_0120_RENDERING_GPU_RESOURCE_PROVIDER_IMPLEMENTATION.md`](T_0120_RENDERING_GPU_RESOURCE_PROVIDER_IMPLEMENTATION.md) – implement GPU resource creation and management for all backends.
- [`T_0121_RENDERING_STANDARD_PASSES_LIBRARY.md`](T_0121_RENDERING_STANDARD_PASSES_LIBRARY.md) – build comprehensive library of standard render passes (shadows, lighting, post-processing).
- [`T_0122_RENDERING_VISIBILITY_CULLING_SYSTEM.md`](T_0122_RENDERING_VISIBILITY_CULLING_SYSTEM.md) – implement frustum culling, occlusion culling, and LOD systems.
- [`T_0123_RENDERING_PIPELINE_STATE_MANAGEMENT.md`](T_0123_RENDERING_PIPELINE_STATE_MANAGEMENT.md) – implement PSO caching, shader management, and state tracking.
- [`T_0124_RENDERING_LIGHTING_SYSTEM.md`](T_0124_RENDERING_LIGHTING_SYSTEM.md) – implement comprehensive lighting with PBR shading, shadows, and light culling.
- [`AI_004_APPLICATION_PROTOTYPING_ENABLEMENT.md`](AI_004_APPLICATION_PROTOTYPING_ENABLEMENT.md) – cross-module initiative aligning rendering, runtime, tools, assets, and benchmarking deliverables.
- [`DC_040_AI_004_CONFIGURATION_SCHEMA_ALIGNMENT.md`](DC_040_AI_004_CONFIGURATION_SCHEMA_ALIGNMENT.md) – cross-module schema alignment.
- [`DC_041_AI_004_KICKOFF_READINESS.md`](DC_041_AI_004_KICKOFF_READINESS.md) – publish kickoff milestone plan and risk ownership.
- [`RE_610_RESEARCH_RENDERING_BASELINE.md`](RE_610_RESEARCH_RENDERING_BASELINE.md) – deliver research-grade rendering preset with telemetry.
- [`RT_320_RUNTIME_PROTOTYPING_HARNESS.md`](RT_320_RUNTIME_PROTOTYPING_HARNESS.md) – build reusable harness with interactive/headless modes and scripting hooks.
- [`RT_321_PROTOTYPING_CASE_STUDY_VALIDATION.md`](RT_321_PROTOTYPING_CASE_STUDY_VALIDATION.md) – deliver reproducible AI-004 case studies with telemetry baselines.
- [`TL_210_EXPERIMENT_SANDBOX_UI.md`](TL_210_EXPERIMENT_SANDBOX_UI.md) – ship ImGui sandbox for experiment configuration and telemetry capture.
- [`AS_330_REFERENCE_DATASET_PACKAGES.md`](AS_330_REFERENCE_DATASET_PACKAGES.md) – curate datasets with manifests, ingestion scripts, and provenance.
- [`CC_310_COMPARATIVE_BENCHMARK_AUTOMATION.md`](CC_310_COMPARATIVE_BENCHMARK_AUTOMATION.md) – automate comparative benchmarking and CI gates.
- [`CC_311_BENCHMARK_VISUALIZATION_INTEGRATION.md`](CC_311_BENCHMARK_VISUALIZATION_INTEGRATION.md) – integrate comparative plots and CI gating for AI-004 benchmarks.

Create new task files as work is planned. Archive completed tasks under a `done/` subdirectory if they remain valuable references.
