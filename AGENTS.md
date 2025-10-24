# Contributor Guidance

## AI Agent Priority Stack

When working as an AI agent, prioritize in this order:

1. **Correctness** – preserve invariants and task acceptance criteria.
2. **Clarity** – maintain documentation, comments, and tests that explain why decisions were made.
3. **Performance** – ensure changes respect the existing profiling budgets and telemetry.
4. **Velocity** – prefer incremental, well-scoped tasks over sweeping refactors.

### Always Do

- Cite every file path or command you reference.
- Follow the session checklist in [`docs/NAVIGATION.md`](docs/NAVIGATION.md) before modifying anything.
- When collaborating across multiple roles, review the coordination guidance in [`agents/AGENTS.md`](agents/AGENTS.md) so hand-offs and context packs stay aligned.
- Update or add tests for every behaviour change. Place C++ coverage under the owning module in `engine/<module>/tests/` and Python coverage under `python/tests/` or `scripts/tests/`.
- Mirror behavioural or dependency changes into module READMEs, module roadmaps, the central roadmap, and relevant task files.
- Escalate missing context by listing the exact files or specifications you require.

### Never Do

- Invent APIs or behaviours that contradict the decision records in [`docs/specs/`](docs/specs/) or the architecture plan.
- Merge changes without aligning task status and documentation.
- Introduce new dependencies without documenting installation and runtime implications.

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
- Follow the conventions in `CODING_STYLE.md` (modern, high-performance C++20+). Do not wrap imports in try/catch blocks. Match the existing naming, formatting, and error-handling patterns.
- Comment complex algorithms with *why* and *how* rationales, not just *what*. Leverage docstrings or Doxygen-style comments for public headers.
- Every new feature must include accompanying unit/integration tests inside the relevant `engine/<module>/tests/` directory and be wired into CTest.
- Keep the aggregated TODO table in `README.md` synchronised with per-module README TODO sections.

## 5. Submission Checklist
- [ ] All affected README files updated to describe the current behaviour, dependencies, and TODO items.
- [ ] Required dependencies documented and, when possible, install scripts updated.
- [ ] Code adheres to `CODING_STYLE.md` and is sufficiently commented.
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
    CODING_STYLE.md
    README.md
    agents/
        00-COMMON-GUARDRAILS.md
        10-Product-Manager.md
        11-Agent-Orchestrator.md
        12-Knowledge-Librarian.md
        13-Research-Scientist.md
        14-Auto-Improver.md
        15-Security-Safety-Gate.md
        16-Community-Maintainer.md
        17-Example-Session.md
        20-Chief-Architect.md
        30-Tech-Lead.md
        40-Rendering-Engineer.md
        50-Geometry-Math-Engineer.md
        60-Physics-Engineer.md
        70-Tools-Build-CI-Engineer.md
        80-Performance-Engineer.md
        90-QA-Test-Engineer.md
        95-Docs-DevRel.md
        98-Release-Manager.md
        99-Reviewer.md
        AGENTS-INDEX.md
        AGENTS-QUICKSTART.md
        AGENTS.md
        TEMPLATES/
            ADR_TEMPLATE.md
            CONTEXT_PACK.md
            ISSUE_TEMPLATE.md
            PR_TEMPLATE.md
            TASK_CARD.md
    docs/
        AGENTIC_WORKFLOW_ENHANCEMENT.md
        ARCHITECTURE.md
        CODEX_PROMPTING_GUIDE.md
        GLOSSARY.md
        HYBRID_WORKFLOW.md
        HYBRID_WORKFLOW_DIAGRAM.md
        HYBRID_WORKFLOW_SUMMARY.md
        NAVIGATION.md
        README_TEMPLATE.md
        ROADMAP.md
        WORKFLOW_COMPARISON.md
        archive/
            README.md
            prints/
                AI-002-STREAMING-GEOMETRY-TELEMETRY.md
                AN-201-IMPLEMENTATION.md
                AS-330-DIAGNOSTICS-SHELL-IMPLEMENTATION.md
                CC-001-TELEMETRY-METRIC-PREFIX.md
                CC-002-3-SHADER-IMPLEMENTATION.md
                CO-150-CO-160-IMPLEMENTATION.md
                CR-125-CR-130-IMPLEMENTATION.md
                CR-135-IMPLEMENTATION.md
                GE-212-GE-220-WORK-DIVISION.md
                IMPLEMENTATION_PROMPT.md
                IO-230-IMPLEMENTATION.md
                MA-110-SIMD-VALIDATION-HARNESS-REVIEW.md
                MA-110-SIMD-VALIDATION-HARNESS.md
                MA-118-SOLVER-STABILITY-DOCUMENTATION.md
                README.md
                RT-005-3-HIERARCHY-DIAGNOSTICS-DOCS.md
                RT-005-4-HIERARCHY-ALERT-THRESHOLDS.md
                RT-006-3-IO-DETECTION-DOCS.md
                SC-220-DOCUMENTATION-REFRESH.md
                SC-225-HIERARCHY-DIAGNOSTICS-SAMPLES.md
                TL-110-TELEMETRY-DOCS-FOLLOWUP.md
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
            workflow-migration/
                DOCUMENTATION_RESTRUCTURE_CHANGELOG.md
                DOCUMENTATION_RESTRUCTURE_PROPOSAL.md
                RESTRUCTURE_SUMMARY.md
        design/
            ANIMATION_GPU_PARALLEL_SAMPLING_BENCHMARK.md
            ARCHITECTURE_IMPROVEMENT_PLAN.md
            ASYNC_STREAMING.md
            ERROR_HANDLING_MIGRATION.md
            GE-212-REMESHING_PARAMETERIZATION_RFP.md
            MATERIAL_PERSISTENCE_STRATEGY.md
            PLUGIN_ARCHITECTURE.md
            RESOURCE_MANAGEMENT.md
            TELEMETRY_INSTRUMENTATION_GUIDE.md
            TELEMETRY_SCHEMA.md
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
                DISPATCHER-EXTENSION-GUIDE.md
                README.md
            core/
                BACKLOG.md
                README.md
            geometry/
                BACKLOG.md
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
                SDL_BACKEND_CHECKLIST.md
            rendering/
                BACKEND_CHECKLIST.md
                BACKLOG.md
                METADATA_SCHEMA.md
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
                BACKLOG.md
                README.md
        prompts/
            ARCHITECTURE-AUDIT.md
            IMPLEMENTATION-PLAYBOOK.md
            MASTER-AGENT-PROMPT.md
            REFACTOR-PLAYBOOK.md
            REVIEW-CHECKLIST.md
        reviews/
            2025-03-22-scene-docs.md
        specs/
            ADR-0003-runtime-frame-graph.md
            ADR-0005-geometry-io-roundtrip.md
            ADR-0006-animation-deformation.md
            AN-240-state-machine-authoring.md
            README.md
        tasks/
            2025-02-17-sprint-06.md
            README.md
            T-0104-runtime-frame-graph-integration.md
            T-0112-geometry-io-roundtrip-hardening.md
            T-0113-animation-runtime-skinning.md
            T-0114-testing-integration-suites.md
            T-0115-assets-async-streaming-mvp.md
            T-0116-rendering-vulkan-resource-translation.md
            T-0117-physics-contact-manifolds.md
            T-0118-testing-framework-upgrade.md
            T-0119-rendering-command-encoder-implementation.md
            T-0120-rendering-gpu-resource-provider-implementation.md
            T-0121-rendering-standard-passes-library.md
            T-0122-rendering-visibility-culling-system.md
            T-0123-rendering-pipeline-state-management.md
            T-0124-rendering-lighting-system.md
            T-0125-math-convenience-rotation-builders.md
            T-0126-math-documentation-alignment.md
            T-0127-math-optional-curve-utilities.md
            T-0128-geometry-frustum-utilities.md
            T-0129-geometry-shape-intersection-coverage.md
    engine/
        CMakeLists.txt
        animation/
            CMakeLists.txt
            include/
                engine/
                    animation/
                        api.hpp
                        deformation/
                            linear_blend_skinning.hpp
                        rigging/
                            rig_binding.hpp
                        skinning/
            src/
                api.cpp
                serialization.cpp
                deformation/
                    linear_blend_skinning.cpp
            tests/
                CMakeLists.txt
                test_blend_tree.cpp
                test_clip_serialization.cpp
                test_module.cpp
                test_rig_binding.cpp
                test_skinning.cpp
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
                        validation.hpp
                        detail/
                            filesystem_utils.hpp
                            reload_utils.hpp
            samples/
            shaders/
            src/
                api.cpp
                graph_asset.cpp
                material_asset.cpp
                mesh_asset.cpp
                point_cloud_asset.cpp
                shader_asset.cpp
                texture_asset.cpp
                validation.cpp
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
            src/
                api.cpp
                dependency_analysis.cpp
            tests/
                CMakeLists.txt
                test_module.cpp
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
                normal_recompute_benchmark.cpp
            include/
                engine/
                    geometry/
                        api.hpp
                        export.hpp
                        random.hpp
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
                        shapes/
                            aabb.hpp
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
                shapes/
                    aabb.cpp
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
                test_shape_interactions.cpp
                test_shapes.cpp
                test_surface_mesh_conversion.cpp
                test_surface_mesh_io.cpp
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
                        cache/
                        exporters/
                        importers/
                            animation.hpp
            signatures/
                geometry_signatures.json
            src/
                animation_importer.cpp
                api.cpp
                geometry_io.cpp
                geometry_io_registry.cpp
                telemetry.cpp
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
                        utils/
                            svd_jacobi.hpp
                            utils.hpp
                            utils_camera.hpp
                            utils_matrix.hpp
                            utils_rotation.hpp
            tests/
                CMakeLists.txt
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
                    sdl_window.cpp
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
                        command_encoder.hpp
                        components.hpp
                        components.inl
                        forward_pipeline.hpp
                        frame_graph.hpp
                        frame_graph_types.hpp
                        gpu_scheduler.hpp
                        material_system.hpp
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
                            opengl/
                                gpu_scheduler.hpp
                            vulkan/
                                gpu_scheduler.hpp
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
                    opengl/
                        command_stream.cpp
                    vulkan/
                        resource_translation.cpp
                resources/
                    recording_gpu_resource_provider.cpp
            tests/
                CMakeLists.txt
                command_encoder_test_utils.hpp
                scheduler_test_utils.hpp
                test_backend_adapters.cpp
                test_backend_validation.cpp
                test_forward_pipeline.cpp
                test_frame_graph.cpp
                test_module.cpp
                test_vulkan_resource_translation.cpp
        runtime/
            CMakeLists.txt
            include/
                engine/
                    runtime/
                        api.hpp
                        diagnostics_bridge.hpp
                        errors.hpp
                        subsystem_registry.hpp
            src/
                api.cpp
                diagnostics_bridge.cpp
                subsystem_registry.cpp
            tests/
                CMakeLists.txt
                test_module.cpp
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
            include/
                engine/
                    tools/
                        api.hpp
                        imgui_helpers.hpp
                        profiling/
                            profiler.hpp
            pipelines/
            profiling/
            src/
                api.cpp
                imgui_helpers.cpp
                profiling/
                    profiler.cpp
            tests/
                CMakeLists.txt
                test_module.cpp
                test_profiler.cpp
    python/
        README.md
        requirements.txt
        engine3g/
            __init__.py
            loader.py
            loader.pyi
            README.md
        tests/
            README.md
            test_loader.py
    scripts/
        __init__.py
        cleanup_redundant_docs.sh
        README.md
        update_agents_tree.py
        validate_docs.py
        ci/
            package_runtime_artifacts.py
            README.md
            run_presets.py
        diagnostics/
            collision_benchmark_report.py
            geometry_normals_benchmark_report.py
            README.md
            runtime_frame_telemetry.py
            streaming_report.py
            telemetry_viewer.py
        lint/
            __init__.py
            error_handling.py
            legacy_error_allowlist.json
        tests/
            test_check_error_handling.py
            test_collision_benchmark_report.py
            test_geometry_normals_benchmark_report.py
            test_package_runtime_artifacts.py
            test_runtime_frame_telemetry.py
            test_streaming_report.py
            test_telemetry_viewer.py
            test_telemetry_viewer_smoke.py
            test_update_agents_tree.py
            test_validate_docs.py
```
<!-- END GENERATED FILE TREE -->
