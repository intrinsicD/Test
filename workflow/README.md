# Test Engine Workspace — Modular agent-driven engine prototype

> Application prototyping platform with curated datasets, deterministic automation, and GPU/runtime convergence checkpoints.

[![Build](#)](#) [![License](#)](#) [![C++20](#)](#) [![Platforms](#)](#)

---

## TL;DR
- Start here (workflow): **[AGENTS.md](./AGENTS.md)**
- Contribute (rules & tools): **[CONTRIBUTING.md](./CONTRIBUTING.md)**
- What’s next (priorities): **[ROADMAP.md](./ROADMAP.md)**
- Tasks queue: **[`/backlog`](./backlog/)** (use `000-template.md`)

---

## Goals
<!-- MIGRATE: high-level goals (bullets). Keep it brief. -->
- Restore end-to-end GPU execution by landing the T-0120/T-0119 rendering milestones.
- Ship the runtime stage planner (RT-410) in lockstep with presentation adapters.
- Keep the AI-004 prototyping workflow reproducible across docs, presets, and datasets.

## Features
<!-- MIGRATE: bullets; focus on user-facing capabilities -->
- Modular C++20 subsystems (animation, geometry, physics, runtime, rendering) with shared headers under `engine::headers`.
- Schema-driven prototyping harness with curated datasets, Dear ImGui sandbox tooling, and telemetry overlays.
- Python bindings (`python/engine3g`) mirroring native APIs plus diagnostics/CI scripts for datasets, benchmarks, and docs validation.

## Repo Layout
````

/                # top-level docs
include/         # public headers
src/             # library sources
apps/            # demos/tools
tests/           # unit tests
bench/           # benchmarks
shaders/         # shaders
data/            # sample assets
docs/            # optional ADR/design notes
backlog/         # tasks (+ archive/)
AGENTS.md
CONTRIBUTING.md
ROADMAP.md

````

## Quickstart

### Prereqs
- Compiler: Clang ≥22, GCC ≥12, or MSVC ≥19.34 (tested with Clang 22.0, GCC 13.2, MSVC 19.38)
- CMake: ≥ 3.20 (validated with 3.28.3) + Ninja ≥1.11 or VS2022 generators
- Optional: Vulkan SDK 1.3.x, CUDA toolkit for compute presets, GLFW system packages (`libxrandr-dev`/`libxinerama-dev`/`libxcursor-dev`/`libxi-dev`) when enabling desktop backends

### Build (Linux)
```bash
cmake --preset linux-gcc-debug
cmake --build --preset linux-gcc-debug
ctest --preset linux-gcc-debug --output-on-failure
````

### Build (Windows)

```powershell
cmake --preset windows-msvc-debug
cmake --build --preset windows-msvc-debug --config Release
ctest --preset windows-msvc-debug -C Release --output-on-failure
```

### Run a demo

```bash
./build/tools/examples/geometry_viewer --help
```

## Agentic Workflow (summary)

See **[AGENTS.md](./AGENTS.md)**.

1. Pick highest-priority ready task → `/docs/backlog/active/*.md`
2. Plan/design per **CONTRIBUTING** standards and attach artefacts (task brief, context package, quality report)
3. Implement with tests/benches using canonical presets
4. PR → review → merge with roadmap/backlog/doc sync
5. Mark task done → archive under `docs/backlog/archive/` → update roadmap + README snapshot

## Architecture at a Glance

<!-- MIGRATE: 5–8 bullets or 1 diagram link -->

* Runtime host advances animation, physics, and rendering via subsystem registry dependency ordering (`engine/runtime`).
* Geometry module owns mesh/point-cloud primitives, spatial indices (kd-tree/octree), and deformation utilities feeding physics + rendering.
* Assets subsystem provides generational handle caches with hot reload telemetry consumed by runtime/tooling.
* Rendering frame-graph compiles deterministic passes; GPU resource provider + command encoder milestones bridge to backend schedulers (Vulkan/OpenGL/DirectX).
* Tools module layers Dear ImGui diagnostics over runtime loops with shared panel registry and profiling utilities.
* Python harness mirrors configuration schema to orchestrate datasets, smoke demos, and comparative benchmarks (`python/engine3g`).

## Performance & Quality Bars

<!-- MIGRATE: your blocking thresholds/targets -->

* Tests must pass; perf regressions exceeding 2% block merges without mitigation.
* Run the canonical preset stack (`cmake --preset linux-gcc-debug`, build, `ctest`, `pytest python/tests scripts/tests`, `python scripts/validate_docs.py`).
* Record benchmark deltas and telemetry artefacts in task briefs/quality reports before requesting review.

## Troubleshooting

<!-- MIGRATE: frequent gotchas + fixes -->

* Missing X11/GLFW headers → install `libxrandr-dev libxinerama-dev libxcursor-dev libxi-dev` or configure with `-DENGINE_ENABLE_GLFW=OFF` for mock backend only.
* Python environment drift → run `python scripts/bootstrap_python_env.py` to sync virtualenv pins before executing automation.
* Docs validation failures → execute `python scripts/validate_docs.py` and update cross-links/navigation tables in `docs/NAVIGATION.md`.

## Contributing & License

* Read **[CONTRIBUTING.md](./CONTRIBUTING.md)** before PRs.
* All changes should reference a `/docs/backlog/*.md` task (active or archived).
* License: Private workspace (coordinate with maintainers for distribution details).

````
