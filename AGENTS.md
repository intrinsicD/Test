# Contributor Guidance

## AI Agent Priority Stack

When working as an AI agent, prioritize in this order:

1. **Correctness** – preserve invariants and task acceptance criteria.
2. **Clarity** – maintain documentation, comments, and tests that explain why decisions were made.
3. **Performance** – ensure changes respect the existing profiling budgets and telemetry.
4. **Velocity** – prefer incremental, well-scoped tasks over sweeping refactors.

## 0. Workflow Blueprint
This file is the **authoritative operating manual** for every AI or human contributor. Keep it open throughout the session and treat each section as an actionable checklist. The workflow is designed as an iterative software-development loop: gather context, commit to a plan, implement, validate, and broadcast results. The subsections below map that loop onto concrete artefacts and responsibilities.

### 0.1 Orientation Principles
- Cite every file path or command you reference.
- Follow the navigation ladder in [§0.2](#02-context-ladder) at the beginning of each session.
- Sync every behavioural or dependency change across source, tests, module READMEs, roadmaps, and backlog artefacts.
- Escalate missing context immediately by naming the unresolved documents or specifications.
- Prefer small, reviewable diffs that keep the quality gates green.

### 0.2 Context Ladder
Load references in this deterministic order before touching code or docs. Capture findings in the task brief.
1. [`README.md`](README.md) – workspace snapshot and module health.
2. [`docs/NAVIGATION.md`](docs/NAVIGATION.md) – documentation index and precedence rules.
3. [`docs/ROADMAP.md`](docs/ROADMAP.md) – strategic initiatives and priority bands.
4. [`docs/backlog/active/<id>.md`](docs/backlog/active/) – task-level acceptance criteria and role roster.
5. Module README(s) under [`docs/modules/`](docs/modules/) – subsystem invariants.
6. Binding decisions under [`docs/specs/ADR-*.md`](docs/specs/) plus relevant design notes.
7. Historical context in [`docs/reviews/`](docs/reviews/) or [`docs/archive/`](docs/archive/) when risk or precedent is unclear.

Record unresolved questions in the context package so the Knowledge Librarian can chase missing pieces.

### 0.3 Deliverable Matrix
| Phase | Primary Roles | Mandatory Artefacts |
| --- | --- | --- |
| Intake & Scoping | Product Manager, Agent Orchestrator | Task brief ([`agents/TEMPLATES/TASK_BRIEF_TEMPLATE.md`](agents/TEMPLATES/TASK_BRIEF_TEMPLATE.md)) with role roster, scope, and risk ledger |
| Context Assembly | Knowledge Librarian, Docs/DevRel | Context package ([`agents/TEMPLATES/CONTEXT_PACKAGE_TEMPLATE.md`](agents/TEMPLATES/CONTEXT_PACKAGE_TEMPLATE.md)) plus curated context ladder notes |
| Execution & Collaboration | Specialist Engineer(s), supporting roles | Implementation plan in task brief, inline commentary, incremental commits/tests |
| Quality Gates | QA/Test, Performance, Safety, Docs/DevRel, Reviewer | Quality report ([`agents/TEMPLATES/QUALITY_REPORT_TEMPLATE.md`](agents/TEMPLATES/QUALITY_REPORT_TEMPLATE.md)) including command evidence |
| Release & Documentation Sync | Release Manager, Docs/DevRel, Knowledge Librarian | Synced documentation, roadmap/backlog status updates, release notes |

### 0.4 Phase Checklists
Each phase has explicit entry/exit criteria. Do not advance until the exit criteria are satisfied and logged in the task brief.

#### Phase 1 – Intake & Scoping
- **Entry:** New task identified or backlog item pulled.
- **Actions:**
  - Validate scope against roadmap priorities and module ownership.
  - Fill in the task brief summary, success criteria, and preliminary role roster.
  - Capture known risks, dependencies, and expected quality gates.
- **Exit:** Task brief approved by the Agent Orchestrator; backlog item linked.

#### Phase 2 – Context Assembly
- **Entry:** Task brief approved; Knowledge Librarian assigned.
- **Actions:**
  - Traverse the context ladder and populate the context package with quotes, links, and open questions.
  - Highlight architectural invariants, telemetry budgets, and testing precedents.
  - Flag missing artefacts to Docs/DevRel and update the task brief communication log.
- **Exit:** Context package reviewed by Specialist Engineer(s); outstanding questions assigned owners.

#### Phase 3 – Execution & Collaboration
- **Entry:** Context package accepted; implementation path agreed.
- **Actions:**
  - Implement changes referencing `CONTRIBUTION.md` standards.
  - Keep the task brief’s decision log current with timestamps.
  - Notify dependent roles (QA, Docs, Performance) of upcoming validation windows.
- **Exit:** Feature/bugfix implemented, tests updated, and code ready for validation.

#### Phase 4 – Quality Gates
- **Entry:** Implementation ready for validation.
- **Actions:**
  - Execute the central build workflow (see [§0.5](#05-quality-instrumentation)).
  - Populate the quality report with command outputs, benchmark deltas, and risk notes.
  - Collect sign-off from each gate owner; unresolved failures block progression.
- **Exit:** All gates marked “Pass” or explicitly deferred with mitigation plans recorded in the task brief and backlog item.

#### Phase 5 – Release & Documentation Sync
- **Entry:** Quality report approved by all gate owners.
- **Actions:**
  - Merge documentation updates, roadmap status, and backlog checklists in the same change set.
  - Release Manager drafts changelog notes and confirms packaging status.
  - Archive context artefacts to `docs/archive/` if long-lived or reusable.
- **Exit:** Release notes published, backlog status updated to “Done”, context artefacts linked for future audits.

### 0.5 Quality Instrumentation
Use this canonical command block across all tasks. Copy it into the task brief and quality report, appending task-specific presets as needed.

```bash
cmake --preset linux-gcc-debug
cmake --build --preset linux-gcc-debug
ctest --preset linux-gcc-debug
pytest python/tests scripts/tests
python scripts/validate_docs.py
```

- Document any deviations (additional presets, sanitizers, dataset generation) alongside the rationale.
- Retain raw logs for QA/Test and Performance sign-off.
- When new automation is required, extend `scripts/ci/` and capture instructions in the quality report.

### 0.6 Coordination Model
- **Communication Ledger:** Log every hand-off and decision in the task brief with timestamps and owners.
- **Async Rhythm:** Daily async updates in the brief; urgent blockers escalate to the Agent Orchestrator for rapid arbitration.
- **Conflict Resolution:** Architectural disputes go to the Chief Architect referencing `docs/specs/ADR-*.md`. Documentation conflicts are resolved by Docs/DevRel, with the Orchestrator mediating ties.
- **Escalation Paths:**
  1. Missing context → Knowledge Librarian.
  2. Architectural ambiguity → Chief Architect.
  3. Tooling/build failures → Build Engineer.
  4. Quality gate disagreements → Agent Orchestrator.

### 0.7 Documentation Integration Checklist
For every task, confirm and log the following in both the task brief and quality report:
1. **Roadmap Alignment:** Link to `docs/ROADMAP.md` and the owning backlog entry.
2. **Module Documentation:** Update the relevant module README using `docs/README_TEMPLATE.md` and record TODO sync status.
3. **Architecture Records:** Amend or add ADRs when behaviour or invariants shift.
4. **Navigation Update:** Register new documents or renamed assets in `docs/NAVIGATION.md`.
5. **Contribution Standards:** Cite applicable sections of `CONTRIBUTION.md` in commit messages or task brief notes.

### 0.8 Guardrails
- **Do not** introduce APIs or behaviours that contradict ADRs or architectural plans without filing and approving a replacement ADR.
- **Do not** merge changes until backlog status, documentation, and templates are updated together.
- **Do not** add dependencies without documenting installation/runtime implications and updating automation scripts where feasible.
- **Do** keep tests in lockstep with behavioural changes (C++ under `engine/<module>/tests/`, Python under `python/tests/` or `scripts/tests/`).
- **Do** cite sources (files, commands, telemetry) in every communication artefact.

### Artefact Overview
- **Roles and responsibilities:** [`agents/ROLES.md`](agents/ROLES.md)
- **Task coordination templates:** [`agents/TEMPLATES/`](agents/TEMPLATES)
- **Contribution standards:** [`CONTRIBUTION.md`](CONTRIBUTION.md)
- **Documentation index:** [`docs/NAVIGATION.md`](docs/NAVIGATION.md)

## Agent Directory Workflow

The [`agents/`](agents/) directory houses the focused artefacts that extend the workflow defined in this guidance file.

### Start Here
1. Identify the responsibilities relevant to your role in [`agents/ROLES.md`](agents/ROLES.md).
2. Use the templates under [`agents/TEMPLATES/`](agents/TEMPLATES) to capture task briefs, context packages, and quality reports.
3. Run `python scripts/update_agents_tree.py` after adding or removing artefacts so repository guidance stays synchronised.

### Maintenance Rules
- Keep links to backlog entries, ADRs, and module documentation accurate.
- Update `agents/ROLES.md` and the templates in the same change when responsibilities shift.
- Legacy role files were removed in favour of the streamlined assets above. Refer to repository history if archival context is required.

### Workflow Change Proposals
- File workflow improvements as backlog items tagged `workflow` and assign the Agent Orchestrator as steward.
- Include a red/green impact analysis and testing implications inside the proposal.
- Update this portal, `agents/ROLES.md`, templates, and affected prompts in the **same change** to keep instructions atomic.
- Run `python scripts/update_agents_tree.py` if directory structure changes accompany the workflow update.

Keep this document authoritative; when the workflow evolves, update it alongside the linked artefacts in the same commit.

## 1. Always Start With the Documentation
- Read the repository root `README.md` before making changes; it summarises the workspace layout, build presets, and current TODO backlog.
- Open [`docs/NAVIGATION.md`](docs/NAVIGATION.md) immediately afterwards. It provides a directory index and links the working agreement in this file with subsystem invariants, task backlogs, and ADRs so contributors follow the same workflow as our AI collaborators.
- When touching any module, review its local `README.md`. Create or update it using `docs/README_TEMPLATE.md` so every directory explains:
  - What the component does and how it relates to neighbouring modules.
  - How to build and run its samples/tests.
  - Its local TODO items and how they map back to the aggregated backlog table in the root README.
- Keep documentation in sync with the implementation. Whenever behaviour, dependencies, or workflows change, update the relevant README(s) and design notes under `docs/` and cross-link them from [`docs/NAVIGATION.md`](docs/NAVIGATION.md).

## 1.1 Architecture Improvement Plan Alignment
- Treat [`docs/ROADMAP.md#architecture-improvement-plan`](docs/ROADMAP.md#architecture-improvement-plan) as the authoritative backlog
  for architectural work. When landing changes that map to a plan identifier, update the checklists, dependencies, and priority
  ordering in that document.
- Keep at-a-glance summaries in the root `README.md`, this guidance file, and any module README in sync with the plan. Update
  them within the same change that edits the plan so contributors always see consistent priorities.
- Reference plan identifiers (e.g., `DC-001`, `AI-003`) in commits, PR descriptions, and documentation where applicable to ease
  traceability for reviewers.

## 2. Environment & Dependencies
- **Compilers**: Use a modern C++20 toolchain (MSVC 19.3x, Clang ≥22, or GCC ≥12). Prefer Clang 22 when compiling locally.
- **CMake & Generators**: Require CMake ≥3.20. Use the provided presets (`cmake --preset <name>`) which default to Ninja generators. Add new presets under `scripts/build/` when new configurations are needed.
- **Python**: Use Python ≥3.12 with `pip`. Manage dependencies in a virtual environment and keep `requirements.txt` up to date.
- **System Packages (Linux/GLFW)**: Install `libxrandr-dev`, `libxinerama-dev`, `libxcursor-dev`, and `libxi-dev` alongside their runtime counterparts. Verify `xrandr` itself is available.
- **Graphics SDKs**: Install the relevant GPU backend SDKs (e.g., Vulkan SDK, DirectX 12 Agility SDK, or platform OpenGL drivers) before enabling those build options.
- Document any additional dependency you introduce in the appropriate README and, if necessary, automate its installation in `scripts/`.

## 3. Build & Run Workflow
1. Configure using the desired preset:
   ```bash
   cmake --preset <preset>
   ```
2. Build the selected configuration:
   ```bash
   cmake --build --preset <preset>
   ```
3. Execute the C++ test suites:
   ```bash
   ctest --preset <preset>
   ```
4. Run the Python unit tests covering both the runtime bindings and maintenance scripts:
   ```bash
   pytest python/tests scripts/tests
   ```
5. Validate that documentation links remain coherent:
   ```bash
   python scripts/validate_docs.py
   ```
6. Extend CI orchestration via the scripts in `scripts/ci/` when you add new presets or automation paths.

## 4. Coding & Testing Standards
- Follow the conventions in `CONTRIBUTION.md` (modern, high-performance C++20+). Do not wrap imports in try/catch blocks. Match the existing naming, formatting, and error-handling patterns.
- Comment complex algorithms with *why* and *how* rationales, not just *what*. Leverage docstrings or Doxygen-style comments for public headers.
- Every new feature must include accompanying unit/integration tests inside the relevant `engine/<module>/tests/` directory and be wired into CTest.
- Keep the aggregated TODO table in `README.md` synchronised with per-module README TODO sections.

## 5. Submission Checklist
- [ ] All affected README files updated to describe the current behaviour, dependencies, and TODO items.
- [ ] Required dependencies documented and, when possible, install scripts updated.
- [ ] Code adheres to `CONTRIBUTION.md` and is sufficiently commented.
- [ ] `cmake --build`, `ctest`, Python tests, and `scripts/validate_docs.py` executed for the relevant presets; record the commands and results.
- [ ] No warnings or regressions introduced in existing tests.

Keep this guidance up to date as workflows evolve so newcomers can build, test, and extend the engine without surprises.


## Repository Hierarchy

Complete repository file hierarchy (excluding hidden entries). Regenerate this block whenever files are added, removed, or reorganised to keep guidance accurate.

<!-- BEGIN GENERATED FILE TREE -->
```text
.
    AGENTS.md
    CMakeLists.txt
    CMakePresets.json
    CONTRIBUTION.md
    README.md
    agents/
        ROLES.md
        TEMPLATES/
            ADR_TEMPLATE.md
            CONTEXT_PACKAGE_TEMPLATE.md
            QUALITY_REPORT_TEMPLATE.md
            TASK_BRIEF_TEMPLATE.md
    assets/
        benchmarks/
            ai004/
                comparative_config.json
                README.md
                data/
                    geometry-baseline/
                        engine_metrics.json
                        reference_metrics.json
                    rendering-debug/
                        engine_metrics.json
                        reference_metrics.json
                reports/
                    comparative_report.html
                    comparative_summary.csv
                    comparative_summary.json
                    geometry-baseline_engine.json
                    geometry-baseline_reference.json
                    rendering-debug_engine.json
                    rendering-debug_reference.json
                    plots/
                        geometry-baseline_average_tick_ms.svg
                        geometry-baseline_fps.svg
                        geometry-baseline_gpu_time_ms.svg
                        rendering-debug_average_tick_ms.svg
                        rendering-debug_fps.svg
                        rendering-debug_overlay_fill_rate.svg
        datasets/
            animation_sample/
                manifest.json
                output_clip.json
                source_clip.json
            case_studies/
                geometry-baseline.json
                index.json
                rendering-debug.json
            remesh_sample/
                manifest.json
                output_mesh.obj
                source_mesh.obj
            rendering_sample/
                manifest.json
                output_mesh.obj
                source_mesh.obj
        hybrid_workflow_dashboard/
            index.html
            tasks.json
    docs/
        ARCHITECTURE.md
        GLOSSARY.md
        NAVIGATION.md
        ROADMAP.md
        architecture/
            module_dependency_graph.dot
            module_dependency_graph.svg
            README.md
        archive/
            ARCHIVE_INDEX.md
            doc_audit_report.json
            README.md
            backlog/
                legacy/
                    README.md
                    modules/
                        GEOMETRY_BACKLOG.md
                        TOOLS_BACKLOG.md
                    tasks/
                        2025-02-17-SPRINT_06.md
                        AI_004_APPLICATION_PROTOTYPING_ENABLEMENT.md
                        AI_004_KICKOFF_BRIEF.md
                        AS_330_REFERENCE_DATASET_PACKAGES.md
                        CC_310_COMPARATIVE_BENCHMARK_AUTOMATION.md
                        CC_311_BENCHMARK_VISUALIZATION_INTEGRATION.md
                        CO_170_RUNTIME_INTEGRATION_SAMPLE.md
                        DC_040_AI_004_CONFIGURATION_SCHEMA_ALIGNMENT.md
                        DC_041_AI_004_KICKOFF_READINESS.md
                        RE_610_RESEARCH_RENDERING_BASELINE.md
                        README.md
                        RT_320_RUNTIME_PROTOTYPING_HARNESS.md
                        RT_321_PROTOTYPING_CASE_STUDY_VALIDATION.md
                        T_0104_RUNTIME_FRAME_GRAPH_INTEGRATION.md
                        T_0112_GEOMETRY_IO_ROUNDTRIP_HARDENING.md
                        T_0113_ANIMATION_RUNTIME_SKINNING.md
                        T_0114_TESTING_INTEGRATION_SUITES.md
                        T_0115_ASSETS_ASYNC_STREAMING_MVP.md
                        T_0116_RENDERING_VULKAN_RESOURCE_TRANSLATION.md
                        T_0117_PHYSICS_CONTACT_MANIFOLDS.md
                        T_0118_TESTING_FRAMEWORK_UPGRADE.md
                        T_0119_RENDERING_COMMAND_ENCODER_IMPLEMENTATION.md
                        T_0120_RENDERING_GPU_RESOURCE_PROVIDER_IMPLEMENTATION.md
                        T_0121_RENDERING_STANDARD_PASSES_LIBRARY.md
                        T_0122_RENDERING_VISIBILITY_CULLING_SYSTEM.md
                        T_0123_RENDERING_PIPELINE_STATE_MANAGEMENT.md
                        T_0124_RENDERING_LIGHTING_SYSTEM.md
                        T_0125_MATH_CONVENIENCE_ROTATION_BUILDERS.md
                        T_0126_MATH_DOCUMENTATION_ALIGNMENT.md
                        T_0127_MATH_OPTIONAL_CURVE_UTILITIES.md
                        T_0128_GEOMETRY_FRUSTUM_UTILITIES.md
                        T_0129_GEOMETRY_SHAPE_INTERSECTION_COVERAGE.md
                        TL_210_EXPERIMENT_SANDBOX_UI.md
            design/
                AI004_CONFIGURATION_LOADER_DESIGN.md
            prints/
                AI_002_STREAMING_GEOMETRY_TELEMETRY.md
                AN_201_IMPLEMENTATION.md
                AS_330_DIAGNOSTICS_SHELL_IMPLEMENTATION.md
                CC_001_TELEMETRY_METRIC_PREFIX.md
                CC_002_3_SHADER_IMPLEMENTATION.md
                CO_150_CO_160_IMPLEMENTATION.md
                CR_125_CR_130_IMPLEMENTATION.md
                CR_135_IMPLEMENTATION.md
                GE_212_GE_220_WORK_DIVISION.md
                IMPLEMENTATION_PROMPT.md
                IO_230_IMPLEMENTATION.md
                MA_110_SIMD_VALIDATION_HARNESS.md
                MA_110_SIMD_VALIDATION_HARNESS_REVIEW.md
                MA_118_SOLVER_STABILITY_DOCUMENTATION.md
                README.md
                RT_005_3_HIERARCHY_DIAGNOSTICS_DOCS.md
                RT_005_4_HIERARCHY_ALERT_THRESHOLDS.md
                RT_006_3_IO_DETECTION_DOCS.md
                SC_220_DOCUMENTATION_REFRESH.md
                SC_225_HIERARCHY_DIAGNOSTICS_SAMPLES.md
                TL_110_TELEMETRY_DOCS_FOLLOWUP.md
            reviews/
                2025-02-17-RUNTIME-TELEMETRY.MD
                2025-02-18-IMPLEMENTATION-PROMPT.MD
                2025-03-17-GEOMETRY-IO-RESULT-MIGRATION.MD
                2025-03-22-SCENE-DOCS.MD
                2025-03-24-ANIMATION-PLANNING.MD
                2025-03-24-AS-320-MATERIAL-PERSISTENCE.MD
                2025-03-28-CORE-PLUGIN-LIFECYCLE.MD
                2025-03-GEOMETRY-STAFFING.MD
                2025-03-TELEMETRY-METRIC-PREFIX.MD
                2025-04-05-COMPUTE-CYCLE-DIAGNOSTICS.MD
                README.md
                legacy/
                    ARCHITECTURE_EVALUATION_EXECUTIVE_SUMMARY.md
                    TASK_COMPLETION_SUMMARY.md
            workflow-migration/
                DOCUMENTATION_RESTRUCTURE_CHANGELOG.md
                DOCUMENTATION_RESTRUCTURE_PROPOSAL.md
                RESTRUCTURE_SUMMARY.md
        backlog/
            README.md
            active/
                PM_510_WEEKLY_INTEGRATION_DEMOS.md
                RT_410_RUNTIME_STAGE_PLANNER.md
                T_0119_COMMAND_ENCODER_INTEGRATION.md
                T_0120_GPU_RESOURCE_PROVIDER.md
                TL_310_EDITOR_FOUNDATIONS.md
            archive/
                AS_330_REFERENCE_DATASET_PACKAGES.md
                CC_310_COMPARATIVE_BENCHMARK_AUTOMATION.md
                CC_311_BENCHMARK_VISUALISATION.md
                DC_040_AI_004_CONFIGURATION_SCHEMA.md
                DC_041_AI_004_KICKOFF_READINESS.md
                PL_240_PLATFORM_FILESYSTEM_WATCHER_GUIDANCE.md
                PM_520_BACKLOG_HYGIENE_REMEDIATION.md
                RE_610_RESEARCH_RENDERING_BASELINE.md
                README.md
                RT_320_RUNTIME_PROTOTYPING_HARNESS.md
                RT_321_PROTOTYPING_CASE_STUDIES.md
                TL_210_EXPERIMENT_SANDBOX_UI.md
        design/
            AI_004_CONFIGURATION_SCHEMA.md
            AI_004_PROTOTYPING_PLAYBOOK.md
            AN_230_2_GPU_TELEMETRY.md
            AN_230_BENCHMARK_HARNESS_DESIGN.md
            ANIMATION_GPU_PARALLEL_SAMPLING_BENCHMARK.md
            ARCHITECTURE_IMPROVEMENT_PLAN.md
            ASYNC_STREAMING.md
            CC_310_BENCHMARK_PLAYBOOK.md
            CO_170_RUNTIME_INTEGRATION_PLAYBOOK.md
            ERROR_HANDLING_MIGRATION.md
            GE_212_REMESHING_PARAMETERIZATION_RFP.md
            MATERIAL_PERSISTENCE_STRATEGY.md
            PLUGIN_ARCHITECTURE.md
            RESOURCE_MANAGEMENT.md
            RT_320_PROTOTYPING_HARNESS.md
            RT_321_CASE_STUDIES.md
            TELEMETRY_INSTRUMENTATION_GUIDE.md
            TELEMETRY_SCHEMA.md
            TL_210_ACCESSIBILITY_CHECKLIST.md
            TL_210_EXPERIMENT_SANDBOX.md
        examples/
            ai004_sample.json
            GEOMETRY_VIEWER_COMPLETION_GUIDE.md
            GEOMETRY_VIEWER_IMPLEMENTATION_SUMMARY.md
            README.md
        modules/
            README.md
            animation/
                BACKLOG.md
                README.md
            assets/
                BACKLOG.md
                README.md
            compute/
                BACKLOG.md
                DISPATCHER_EXTENSION_GUIDE.md
                README.md
            core/
                BACKLOG.md
                README.md
            geometry/
                README.md
            io/
                BACKLOG.md
                DETECTION_FUZZING_PLAYBOOK.md
                README.md
            math/
                BACKLOG.md
                FORMAT_CONVERSIONS.md
                README.md
                SOLVER_STABILITY.md
            physics/
                BACKLOG.md
                README.md
            platform/
                BACKLOG.md
                README.md
            rendering/
                BACKEND_CHECKLIST.md
                BACKLOG.md
                METADATA_SCHEMA.md
                PROGRESS_REPORT.md
                QUICKSTART.md
                README.md
            runtime/
                ASYNC_STREAMING_INTEGRATION.md
                BACKLOG.md
                DIAGNOSTICS.md
                README.md
            scene/
                BACKLOG.md
                DIAGNOSTICS.md
                README.md
            tools/
                README.md
        prompts/
            ARCHITECTURE_AUDIT.md
            IMPLEMENTATION_PLAYBOOK.md
            REFACTOR_PLAYBOOK.md
            REVIEW_CHECKLIST.md
        reviews/
            APPLICATION_FRAMEWORK_INDEX.md
            APPLICATION_FRAMEWORK_PHASE1_COMPLETE.md
            APPLICATION_FRAMEWORK_PHASE2_COMPLETE.md
            APPLICATION_FRAMEWORK_PROPOSAL.md
            APPLICATION_READINESS_ASSESSMENT.md
            ARCHITECTURE_AUDIT.md
            COMPREHENSIVE_ARCHITECTURE_EVALUATION.md
            DATE_REMOVAL_COMPLETE.md
            DOC_HYGIENE_COMPLETION_SUMMARY.md
            DOC_NAMING_STANDARDIZATION_COMPLETE.md
            GEOMETRY_VIEWER_ARCHITECTURE_ANALYSIS.md
            GEOMETRY_VIEWER_RENDERING_GAPS.md
            MISSING_COMPONENTS_SUMMARY.md
            ROADMAP_DIRECTION_REVIEW.md
            SCENE_DOCS.md
        specs/
            ADR_0003_RUNTIME_FRAME_GRAPH.md
            ADR_0005_GEOMETRY_IO_ROUNDTRIP.md
            ADR_0006_ANIMATION_DEFORMATION.md
            ADR_0007_AI_004_CONFIGURATION_SCHEMA.md
            ADR_0008_RUNTIME_MAIN_LOOP_AND_TOOLING.md
            AN_240_STATE_MACHINE_AUTHORING.md
            README.md
        templates/
            README_TEMPLATE.md
            RESEARCH_PAPER_TEMPLATE.md
    engine/
        CMakeLists.txt
        animation/
            CMakeLists.txt
            include/
                engine/
                    animation/
                        api.hpp
                        benchmarking/
                            statistics.hpp
                            telemetry.hpp
                        deformation/
                            linear_blend_skinning.hpp
                        rigging/
                            rig_binding.hpp
                        skinning/
            src/
                api.cpp
                serialization.cpp
                benchmarking/
                    statistics.cpp
                    telemetry.cpp
                deformation/
                    linear_blend_skinning.cpp
            tests/
                CMakeLists.txt
                test_benchmark_statistics.cpp
                test_benchmark_telemetry.cpp
                test_blend_tree.cpp
                test_clip_serialization.cpp
                test_module.cpp
                test_rig_binding.cpp
                test_skinning.cpp
            tools/
                CMakeLists.txt
                benchmark_driver/
                    benchmark_driver.cpp
                    CMakeLists.txt
                    README.md
        assets/
            CMakeLists.txt
            include/
                engine/
                    assets/
                        api.hpp
                        async.hpp
                        graph_asset.hpp
                        handles.hpp
                        material_asset.hpp
                        mesh_asset.hpp
                        point_cloud_asset.hpp
                        shader_asset.hpp
                        texture_asset.hpp
                        texture_decoder.hpp
                        validation.hpp
                        detail/
                            cache_common.hpp
                            filesystem_utils.hpp
                            reload_utils.hpp
            samples/
                hot_reload_failure.obj
                quad.obj
                test_mesh.obj
                test_mesh_corrupted.obj
                triangle.obj
            shaders/
            src/
                api.cpp
                graph_asset.cpp
                material_asset.cpp
                mesh_asset.cpp
                point_cloud_asset.cpp
                shader_asset.cpp
                texture_asset.cpp
                texture_decoder.cpp
                validation.cpp
                decoders/
                    stb_texture_decoder.cpp
            tests/
                CMakeLists.txt
                test_assets.cpp
                test_async.cpp
                test_handle_validation.cpp
                test_module.cpp
        compute/
            CMakeLists.txt
            cuda/
                CMakeLists.txt
                include/
                    engine/
                        compute/
                            cuda/
                                api.hpp
                src/
                    api.cpp
                tests/
                    CMakeLists.txt
                    test_module.cpp
            include/
                engine/
                    compute/
                        api.hpp
                        dependency_analysis.hpp
            samples/
                CMakeLists.txt
                runtime_dispatch_demo/
                    CMakeLists.txt
                    queue_assignment.hpp
                    README.md
                    runtime_dispatch_demo.cpp
                    workload_configuration.cpp
                    workload_configuration.hpp
            src/
                api.cpp
                dependency_analysis.cpp
            tests/
                CMakeLists.txt
                test_module.cpp
                test_runtime_dispatch_queue_assignment.cpp
                test_runtime_dispatch_workload_configuration.cpp
        core/
            CMakeLists.txt
            include/
                engine/
                    core/
                        api.hpp
                        application/
                        configuration/
                        diagnostics/
                            error.hpp
                            result.hpp
                        ecs/
                            component_storage.hpp
                            entity_id.hpp
                            registry.hpp
                            system.hpp
                        memory/
                            resource_pool.hpp
                        parallel/
                        plugin/
                            isubsystem_interface.hpp
                        runtime/
                        telemetry/
                            schema.hpp
                        threading/
                            io_thread_pool.hpp
            src/
                api.cpp
                ecs/
                    registry.cpp
                    system.cpp
                telemetry/
                    schema.cpp
                threading/
                    io_thread_pool.cpp
            tests/
                CMakeLists.txt
                ecs_registry_tests.cpp
                io_thread_pool_tests.cpp
                resource_pool_tests.cpp
                telemetry_schema_tests.cpp
                test_module.cpp
        geometry/
            CMakeLists.txt
            benchmarks/
                CMakeLists.txt
                frustum_culling_benchmark.cpp
                normal_recompute_benchmark.cpp
                shape_intersection_benchmark.cpp
            include/
                engine/
                    geometry/
                        api.hpp
                        export.hpp
                        random.hpp
                        remesh.hpp
                        shapes.hpp
                        telemetry.hpp
                        csg/
                        decimation/
                        deform/
                            linear_blend_skinning.hpp
                        graph/
                            graph.hpp
                        kdtree/
                            kdtree.hpp
                        mesh/
                            halfedge_mesh.hpp
                            surface_mesh_conversion.hpp
                        octree/
                            octree.hpp
                        point_cloud/
                            point_cloud.hpp
                        properties/
                            property_handle.hpp
                            property_registry.hpp
                            property_set.hpp
                        remesh/
                            deviation.hpp
                            errors.hpp
                            remesh.hpp
                            telemetry.hpp
                        shapes/
                            aabb.hpp
                            capsule.hpp
                            cylinder.hpp
                            ellipsoid.hpp
                            frustum.hpp
                            line.hpp
                            obb.hpp
                            plane.hpp
                            ray.hpp
                            segment.hpp
                            sphere.hpp
                            triangle.hpp
                        surfaces/
                        topology/
                            surface_curvature.hpp
                            surface_topology.hpp
                        utils/
                            bounded_heap.hpp
                            circulators.hpp
                            connectivity.hpp
                            iterators.hpp
                            ranges.hpp
                            shape_interactions.hpp
                        uv/
                        volumetric/
            src/
                api.cpp
                telemetry.cpp
                deform/
                    linear_blend_skinning.cpp
                graph/
                    graph.cpp
                    graph_io.cpp
                mesh/
                    halfedge_mesh.cpp
                    halfedge_mesh_io.cpp
                    surface_mesh_conversion.cpp
                point_cloud/
                    point_cloud.cpp
                    point_cloud_io.cpp
                properties/
                    property_handle.cpp
                    property_registry.cpp
                remesh/
                    deviation.cpp
                    remesh.cpp
                    telemetry.cpp
                shapes/
                    aabb.cpp
                    capsule.cpp
                    cylinder.cpp
                    ellipsoid.cpp
                    frustum.cpp
                    line.cpp
                    obb.cpp
                    plane.cpp
                    random.cpp
                    ray.cpp
                    segment.cpp
                    sphere.cpp
                    triangle.cpp
                topology/
                    surface_curvature.cpp
                    surface_topology.cpp
                utils/
                    shape_interactions.cpp
            tests/
                CMakeLists.txt
                test_deformation.cpp
                test_frustum.cpp
                test_graph.cpp
                test_graph_io.cpp
                test_halfedge_io.cpp
                test_halfedge_mesh.cpp
                test_kdtree.cpp
                test_module.cpp
                test_octree.cpp
                test_point_cloud.cpp
                test_property_registry.cpp
                test_remesh_adaptive.cpp
                test_remesh_cli.cpp
                test_remesh_feature_preserving.cpp
                test_remesh_parameterization.cpp
                test_remesh_request.cpp
                test_remesh_telemetry.cpp
                test_remesh_uniform.cpp
                test_shape_interactions.cpp
                test_shapes.cpp
                test_surface_curvature.cpp
                test_surface_deviation.cpp
                test_surface_mesh_conversion.cpp
                test_surface_mesh_io.cpp
                test_surface_topology.cpp
            tools/
                CMakeLists.txt
                geometry_remesh.cpp
                remesh_cli.cpp
                remesh_cli.hpp
        io/
            CMakeLists.txt
            include/
                engine/
                    io/
                        api.hpp
                        errors.hpp
                        geometry_io.hpp
                        geometry_io_registry.hpp
                        telemetry.hpp
                        detail/
                            geometry_io_common.hpp
                        exporters/
                            graph.hpp
                            mesh.hpp
                            point_cloud.hpp
                        importers/
                            animation.hpp
                            graph.hpp
                            mesh.hpp
                            point_cloud.hpp
            signatures/
                geometry_signatures.json
            src/
                animation_importer.cpp
                api.cpp
                geometry_io.cpp
                geometry_io_plugins.cpp
                geometry_io_registry.cpp
                telemetry.cpp
                detail/
                    geometry_io_common.cpp
                exporters/
                    graph_exporters.cpp
                    mesh_exporters.cpp
                    point_cloud_exporters.cpp
                importers/
                    graph_importers.cpp
                    mesh_importers.cpp
                    point_cloud_importers.cpp
            tests/
                CMakeLists.txt
                geometry_io_corpus_tests.cpp
                geometry_io_detection_fuzz.cpp
                geometry_io_registry_tests.cpp
                geometry_io_telemetry_tests.cpp
                test_animation_importer.cpp
                test_geometry_io.cpp
                test_module.cpp
                corpus/
                    geometry_detection/
                        graph_ascii.ply
                        graph_edgelist.txt
                        invalid_notply_header.ply
                        invalid_truncated_header.ply
                        mesh_ascii.ply
                        mesh_ascii.stl
                        mesh_simple.off
                        mesh_triangle.obj
                        point_cloud_ascii.pcd
                        point_cloud_ascii.ply
                        point_cloud_basic.xyz
                        README.md
        math/
            CMakeLists.txt
            include/
                engine/
                    math/
                        common.hpp
                        conversions.hpp
                        math.hpp
                        matrix.hpp
                        quaternion.hpp
                        solvers.hpp
                        sparse_matrix.hpp
                        transform.hpp
                        vector.hpp
                        telemetry/
                            conversion_telemetry.hpp
                        utils/
                            svd_jacobi.hpp
                            utils.hpp
                            utils_camera.hpp
                            utils_matrix.hpp
                            utils_rotation.hpp
            tests/
                CMakeLists.txt
                test_conversion_telemetry.cpp
                test_math.cpp
                test_math_simd.cpp
                test_solvers.cpp
        physics/
            CMakeLists.txt
            benchmarks/
                CMakeLists.txt
                collision_benchmark.cpp
            include/
                engine/
                    physics/
                        api.hpp
                        collision/
                        dynamics/
            src/
                api.cpp
                collisions.cpp
            tests/
                CMakeLists.txt
                test_module.cpp
        platform/
            CMakeLists.txt
            include/
                engine/
                    platform/
                        api.hpp
                        filesystem/
                            filesystem.hpp
                            watcher.hpp
                        input/
                            input_state.hpp
                        windowing/
                            window.hpp
                            window_console.hpp
            src/
                api.cpp
                filesystem/
                    filesystem.cpp
                    watcher.cpp
                input/
                    input_state.cpp
                windowing/
                    glfw_window.cpp
                    glfw_window_stub.cpp
                    mock_window.cpp
                    window_base.cpp
                    window_base.hpp
                    window_console.cpp
                    window_system.cpp
            tests/
                CMakeLists.txt
                filesystem_tests.cpp
                filesystem_watcher_tests.cpp
                input_state_tests.cpp
                test_module.cpp
                window_console_tests.cpp
                window_system_tests.cpp
                window_test_app.cpp
        rendering/
            CMakeLists.txt
            include/
                engine/
                    rendering/
                        api.hpp
                        camera.hpp
                        camera_controllers.hpp
                        command_encoder.hpp
                        components.hpp
                        components.inl
                        forward_pipeline.hpp
                        frame_graph.hpp
                        frame_graph_types.hpp
                        gpu_scheduler.hpp
                        material_system.hpp
                        presentation_backend.hpp
                        render_pass.hpp
                        runtime_submission.hpp
                        backend/
                            native_scheduler_base.hpp
                            stub_gpu_scheduler_base.hpp
                            validation.hpp
                            directx12/
                                gpu_scheduler.hpp
                            metal/
                                gpu_scheduler.hpp
                            mock/
                                presentation_backend.hpp
                            opengl/
                                command_encoder.hpp
                                gpu_scheduler.hpp
                                immediate_command_stream.hpp
                                presentation_backend.hpp
                                render_resource_provider.hpp
                                resource_provider.hpp
                                runtime_adapter.hpp
                            vulkan/
                                command_encoder.hpp
                                gpu_scheduler.hpp
                                resource_provider.hpp
                                resource_translation.hpp
                                vulkan_stub.hpp
                        lighting/
                        materials/
                            shaders/
                                common/
                                glsl/
                                hlsl/
                            textures/
                        passes/
                        pipeline/
                            research_baseline.hpp
                            research_baseline_telemetry.hpp
                        resources/
                            recording_gpu_resource_provider.hpp
                            resource_provider.hpp
                            synchronization.hpp
                            buffers/
                            samplers/
                        visibility/
            src/
                api.cpp
                forward_pipeline.cpp
                frame_graph.cpp
                material_system.cpp
                backend/
                    validation.cpp
                    mock/
                        presentation_backend.cpp
                    opengl/
                        command_encoder.cpp
                        command_stream.cpp
                        immediate_command_stream.cpp
                        presentation_backend.cpp
                        render_resource_provider.cpp
                        resource_provider.cpp
                        runtime_adapter.cpp
                    vulkan/
                        command_encoder.cpp
                        resource_provider.cpp
                        resource_translation.cpp
                pipeline/
                    research_baseline.cpp
                    research_baseline_telemetry.cpp
                resources/
                    recording_gpu_resource_provider.cpp
            tests/
                CMakeLists.txt
                command_encoder_test_utils.hpp
                scheduler_test_utils.hpp
                test_backend_adapters.cpp
                test_backend_validation.cpp
                test_camera.cpp
                test_forward_pipeline.cpp
                test_frame_graph.cpp
                test_module.cpp
                test_opengl_command_encoder.cpp
                test_opengl_render_resource_provider.cpp
                test_opengl_resource_provider.cpp
                test_opengl_runtime_adapter.cpp
                test_research_baseline.cpp
                test_vulkan_command_encoder.cpp
                test_vulkan_gpu_scheduler.cpp
                test_vulkan_resource_provider.cpp
                test_vulkan_resource_translation.cpp
        runtime/
            CMakeLists.txt
            include/
                engine/
                    runtime/
                        api.hpp
                        application.hpp
                        config_schema.hpp
                        diagnostics_bridge.hpp
                        errors.hpp
                        loop.hpp
                        loop_inspector.hpp
                        render_submission.hpp
                        subsystem_registry.hpp
            samples/
                CMakeLists.txt
                prototype_harness.cpp
                README.md
            src/
                api.cpp
                application.cpp
                config_schema.cpp
                diagnostics_bridge.cpp
                loop.cpp
                loop_inspector.cpp
                subsystem_registry.cpp
            tests/
                CMakeLists.txt
                test_config_schema.cpp
                test_loop.cpp
                test_module.cpp
                test_opengl_presentation_backend.cpp
        scene/
            CMakeLists.txt
            include/
                engine/
                    scene/
                        api.hpp
                        components.hpp
                        errors.hpp
                        scene.hpp
                        systems.hpp
                        validation.hpp
                        components/
                            hierarchy.hpp
                            name.hpp
                            transform.hpp
                        graph/
                            scene_graph_validator.hpp
                        serialization/
                            serializer.hpp
                        systems/
                            hierarchy_system.hpp
                            registry.hpp
                            transform_system.hpp
            samples/
                CMakeLists.txt
                hierarchy_diagnostics_sample.cpp
                README.md
                data/
                    invalid_hierarchy.scene
                    valid_hierarchy.scene
            src/
                api.cpp
                scene.cpp
                scene_graph_validator.cpp
                validation.cpp
                serialization/
                    serializer.cpp
                systems/
                    hierarchy_system.cpp
                    registry.cpp
                    transform_system.cpp
            tests/
                CMakeLists.txt
                scene_destruction_tests.cpp
                test_components.cpp
                test_module.cpp
                test_scene_graph_validator.cpp
                test_serialization.cpp
                test_systems.cpp
                test_validation.cpp
        tests/
            integration/
                CMakeLists.txt
                README.md
                test_runtime_integration.cpp
            performance/
            unit/
        tools/
            CMakeLists.txt
            editor/
            examples/
                CMakeLists.txt
                geometry_viewer.cpp
                README.md
            include/
                engine/
                    tools/
                        api.hpp
                        imgui_helpers.hpp
                        imgui/
                            panel_registry.hpp
                        profiling/
                            profiler.hpp
                        sandbox/
                            benchmark_runner.hpp
                            configuration_loader.hpp
                            experiment_sandbox.hpp
            pipelines/
            profiling/
            src/
                api.cpp
                imgui_helpers.cpp
                imgui/
                    panel_registry.cpp
                profiling/
                    profiler.cpp
                sandbox/
                    benchmark_runner.cpp
                    configuration_loader.cpp
                    experiment_sandbox.cpp
            tests/
                CMakeLists.txt
                test_benchmark_runner.cpp
                test_experiment_sandbox.cpp
                test_module.cpp
                test_panel_registry.cpp
                test_profiler.cpp
                test_sandbox_configuration_loader.cpp
    hybrid_workflow/
        AGENTS.md
        COMPLETE.md
        CONTRIBUTING.md
        IMPLEMENTATION_SUMMARY.md
        MIGRATION.md
        QUICK_REFERENCE.md
        README.md
        ROADMAP.md
        task_status.py
        backlog/
            000-template.md
            AI-004-kickoff-brief.md
            PM-510-weekly-integration-demos.md
            RT-410-runtime-stage-planner.md
            SPRINT-11-alignment.md
            T-0119-command-encoder-integration.md
            T-0120-gpu-resource-provider.md
            TL-310-editor-foundations.md
            TL-320-task-dashboard.md
            archive/
                DC-050-workflow-migration.md
    python/
        README.md
        requirements.txt
        engine3g/
            __init__.py
            case_studies.py
            config_schema.py
            config_schema.pyi
            loader.py
            loader.pyi
            prototype_harness.py
            README.md
        tests/
            _helpers.py
            README.md
            test_case_studies.py
            test_config_schema.py
            test_dataset_ingestion.py
            test_loader.py
            test_prototype_harness.py
    scripts/
        __init__.py
        bootstrap_python_env.py
        cleanup_redundant_docs.sh
        generate_dependency_graph.py
        README.md
        rename_docs_to_uppercase.py
        update_agents_tree.py
        validate_ai004_config.py
        validate_docs.py
        benchmarks/
            __init__.py
            run_comparative_benchmarks.py
        ci/
            package_runtime_artifacts.py
            README.md
            run_comparative_smoke.py
            run_presets.py
        datasets/
            __init__.py
            ingest_dataset.py
        diagnostics/
            animation_sampling_report.py
            collision_benchmark_report.py
            compute_dispatch_benchmark.py
            compute_dispatch_report.py
            geometry_normals_benchmark_report.py
            README.md
            runtime_frame_telemetry.py
            streaming_report.py
            telemetry_viewer.py
        lint/
            __init__.py
            error_handling.py
            legacy_error_allowlist.json
        prototyping/
            __init__.py
            run_prototype_harness.py
        tests/
            test_animation_sampling_report.py
            test_bootstrap_python_env.py
            test_check_error_handling.py
            test_collision_benchmark_report.py
            test_compute_dispatch_benchmark.py
            test_compute_dispatch_report.py
            test_dashboard.py
            test_generate_dependency_graph.py
            test_geometry_normals_benchmark_report.py
            test_ingest_dataset.py
            test_package_runtime_artifacts.py
            test_run_comparative_benchmarks.py
            test_run_comparative_smoke.py
            test_runtime_frame_telemetry.py
            test_streaming_report.py
            test_telemetry_viewer.py
            test_telemetry_viewer_smoke.py
            test_update_agents_tree.py
            test_validate_ai004_config.py
            test_validate_docs.py
        workflow/
            __init__.py
            dashboard.py
            report_hybrid_status.py
    telemetry/
        frame_timings.json
    workflow/
        AGENTS.md
        CONTRIBUTING.md
        README.md
        ROADMAP.md
        backlog/
            000-template.md
```
<!-- END GENERATED FILE TREE -->
