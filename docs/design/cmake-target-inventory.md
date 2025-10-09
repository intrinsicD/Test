# CMake Target Inventory

## Overview

This document enumerates the CMake targets that originate from the root `CMakeLists.txt`, the `engine/**/CMakeLists.txt` hierarchy, and the automation scaffolding under `scripts/`. The goal is to make the dependency structure explicit and highlight gaps or potential refactors before introducing additional subsystems.

The root build now defines two cross-cutting interface targets:

- `engine::project_options` – propagates the required C++20 feature set and compiler-specific switches such as `-stdlib=libc++` when building with Clang.
- `engine::headers` – aggregates the public header directories registered by each engine module so that tooling, integration tests, or external consumers can attach to the entire header surface with a single dependency.

All module libraries link against `engine::project_options`, and each module registers its public include directory through `engine_apply_module_defaults`, which simultaneously forwards that directory to `engine::headers`.

## Engine Libraries

| Target | Type | Direct Dependencies |
| --- | --- | --- |
| `engine_animation` | SHARED/STATIC | `engine::project_options`, `engine_math` |
| `engine_assets` | SHARED/STATIC | `engine::project_options`, `engine_io` |
| `engine_compute` | SHARED/STATIC | `engine::project_options`, `engine_math`, `engine_compute_cuda` |
| `engine_compute_cuda` | SHARED/STATIC | `engine::project_options`, `engine_math` |
| `engine_core` | SHARED/STATIC | `engine::project_options`, `EnTT::EnTT`, `spdlog::spdlog_header_only`, `imgui::imgui` |
| `engine_geometry` | SHARED/STATIC | `engine::project_options`, `engine_math` |
| `engine_io` | SHARED/STATIC | `engine::project_options`, `engine_geometry` |
| `engine_math` | INTERFACE | `engine::project_options` |
| `engine_physics` | SHARED/STATIC | `engine::project_options`, `engine_math` |
| `engine_platform` | SHARED/STATIC | `engine::project_options`, `glfw` *or* `glfw_shared` (chosen at configure time) |
| `engine_rendering` | SHARED/STATIC | `engine::project_options`, `engine_core`, `engine_assets`, `engine_platform`, `engine_scene` |
| `engine_runtime` | SHARED/STATIC | `engine::project_options`, `engine_animation`, `engine_assets`, `engine_compute`, `engine_compute_cuda`, `engine_core`, `engine_geometry`, `engine_io`, `engine_math`, `engine_physics`, `engine_platform`, `engine_rendering`, `engine_scene` |
| `engine_scene` | SHARED/STATIC | `engine::project_options`, `EnTT::EnTT`, `engine_core`, `engine_math` |

`engine::headers` collects the include directories contributed by each module (including `engine_compute_cuda`). Consumers that only need the headers can link to this interface target without incurring the object code dependencies.

## Tests and Utilities

All module-specific test executables share a common shape after the refactor: they link against the corresponding module library, the GoogleTest main target, and `engine::project_options` to inherit the same compile feature requirements.

| Target | Purpose |
| --- | --- |
| `engine_animation_tests` | Animation unit tests |
| `engine_assets_tests` | Asset pipeline smoke tests |
| `engine_compute_tests` | CPU compute façade tests |
| `engine_compute_cuda_tests` | CUDA compute façade tests |
| `engine_core_tests` | ECS and core service tests |
| `engine_geometry_tests` | Geometry primitives and utilities |
| `engine_geometry_shape_interactions_tests` | Focused geometry interaction suite |
| `engine_io_tests` | IO registries |
| `engine_math_tests` | Header-only math validation |
| `engine_physics_tests` | Physics utilities |
| `engine_platform_tests` | Windowing abstractions |
| `engine_platform_window_app` | Manual smoke harness for platform layer |
| `engine_rendering_tests` | Rendering orchestrator checks |
| `engine_runtime_tests` | Runtime aggregation |
| `engine_scene_tests` | Scene graph tests |

The root build also continues to define the `docs` custom target that drives `scripts/validate_docs.py` when Python is available.

## Third-Party Integrations

Third-party libraries are brought in through their own packages (`EnTT::EnTT`, `spdlog::spdlog_header_only`, `imgui::imgui`, `glfw`/`glfw_shared`) and remain isolated from engine internals except through explicit `target_link_libraries` usage requirements.

The FetchContent fallbacks for EnTT and GLFW honour the cache variables emitted by the new presets, ensuring consistent feature toggles (`GLFW_BUILD_*` options, shared-library builds, Python enablement) regardless of whether dependencies are vendored or fetched at configure time.

## Dependency Graph Summary

- There are no cyclic dependencies among the engine modules. The longest chain currently flows from `engine_runtime` → `engine_compute` → `engine_compute_cuda` → `engine_math` and from `engine_assets` → `engine_io` → `engine_geometry` → `engine_math`.
- `engine_core`, `engine_scene`, and `engine_rendering` form the central hub for higher-level subsystems; they all terminate on `engine_runtime`.
- `engine_platform` sits at the edge of the graph and only depends on the windowing backend exposed by GLFW.

## Identified Gaps

1. **CUDA hard dependency** – `engine_compute` and `engine_runtime` always pull in `engine_compute_cuda`. A future portability improvement would add a configuration option that allows CPU-only builds to skip CUDA entirely.
2. **Platform backend selection** – the build currently assumes GLFW is the active windowing provider. SDL stubs exist in the source tree, but there is no preset toggle or optional dependency guarding its inclusion.
3. **Header aggregation visibility** – while `engine::headers` simplifies discovery for tooling, external consumers that only link to a subset of modules may inadvertently gain include visibility into unrelated subsystems. Monitoring include hygiene in downstream targets is recommended.

No other structural cycles were found during this audit.
