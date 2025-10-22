# Architecture Audit Prompt

Use this prompt to execute deep architectural reviews of the Test Engine codebase. It focuses on system-wide consistency, debt
triage, and roadmap alignment rather than single-change pull-request reviews.

## When to Use
- Quarterly or milestone health checks.
- After landing multiple related initiatives that could shift invariants.
- When onboarding new contributors to architectural ownership roles.
- Whenever telemetry or review findings hint at architectural drift.

## Prerequisites Checklist
- [ ] Review [`../README.md`](../README.md), [`architecture.md`](../architecture.md), and [`agents.md`](../agents.md) to
      refresh invariants and reviewer expectations.
- [ ] Inspect the architecture improvement plan tables in
      [`../ROADMAP.md`](../ROADMAP.md#architecture-improvement-plan) and gather linked tasks/prints.
- [ ] Open relevant specs under [`../specs/`](../specs/) and design notes in [`../design/`](../design/).
- [ ] Collect module READMEs/ROADMAPs for the subsystems in scope.
- [ ] Confirm required tooling is available (`cmake`, `python`, telemetry scripts).

## Scope Options
- **Full-System Audit** – examine every module against invariants and roadmap intent.
- **Module-Focused Audit** – deep dive into one or more modules plus their dependencies.
- **Cross-Cutting Concern Audit** – follow a theme (e.g., error handling, resource lifetime, observability) across modules.
- **Dependency Audit** – verify layering and plugin boundaries using build graphs.

## Prompt Template
```
You are conducting a Test Engine architecture audit.

Do NOT disclose chain-of-thought. Respond only with requested artefacts and cited evidence.

**Global Guardrails**
- Preserve determinism, handle safety, and documentation discipline described in `docs/architecture.md`.
- Cite files, specs, and roadmap entries that support each finding.
- Prefer actionable tasks over vague advice; align follow-ups with roadmap/task IDs.

**Scope Configuration**
- Audit Mode: {Full-System | Module[{names}] | Theme[{concern}] | Dependency}
- Timebox: {e.g., 2h, 4h}
- Initiatives in Focus: {IDs from roadmap}
- Reference Artefacts: {list of READMEs/specs/tasks consulted}

**Phase A — Baseline**
1. Summarise the current architecture improvement plan items related to the scope.
2. Capture dependency graph observations (targets, module layering, plugins).
3. Note recent initiatives or merges that triggered this audit.

**Phase B — Invariant Verification**
For each invariant in `docs/architecture.md`, record status and evidence.

### Deterministic Scheduler
- [ ] Frame-graph compilation deterministic for identical inputs.
- [ ] Resource transitions explicit and validated.
- [ ] Backend implementations cannot reorder passes.
- [ ] Regression: `frame_graph_serialization_is_deterministic` present and passing.

### Resource Ownership (`AI-001`)
- [ ] Asset/scene/runtime handles use `ResourceHandle<Tag>`.
- [ ] Backed by `ResourcePool` with generation counters.
- [ ] Debug builds trap invalid handle access.
- [ ] Regression: `stale_handle_detection` passes.

### Geometry Fidelity
- [ ] Spatial structures (kd-tree, octree) stay in sync with mesh mutations.
- [ ] Bounds/centroids recomputed before publication.
- [ ] IO round-trips preserve topology/attributes.
- [ ] Regression: `geometry_roundtrip_preserves_checksum` passes.

### Physics Integration
- [ ] Mass/damping clamped to valid ranges; static bodies ignore forces.
- [ ] Substep progression monotonic.
- [ ] Collision detection delegates to geometry predicates.
- [ ] Regression: `physics_static_bodies_ignore_gravity` passes.

### Documentation Discipline
- [ ] Every affected module README matches `docs/README_TEMPLATE.md`.
- [ ] Module and central roadmaps cross-reference current status.
- [ ] ADRs/design notes cover major decisions.

**Phase C — Module Deep Dives**
For each module in scope capture:
- Module: {name}
- Public API Surface: headers/symbol counts and stability classification.
- Dependencies: direct + transitive, unexpected couplings, plugin boundaries.
- Internal Complexity: LOC, cyclomatic notes, test/benchmark coverage.
- Documentation Health: README/ROADMAP freshness, TODO accuracy.
- Telemetry & Tooling: emitted metrics, diagnostics bridge coverage.

**Phase D — Cross-Cutting Concerns**
Evaluate themes relevant to the scope (error handling, resource lifetime, observability, performance). Document violations with
file references and recommended fixes.

**Phase E — Findings & Follow-Up**
1. Classify each issue: 🔴 Critical, ⚠️ Warning, 💡 Suggestion.
2. Map blocking items to roadmap/task IDs or create new tasks.
3. Highlight dependencies and owners for each follow-up.

==============================================================================
OUTPUT SCHEMA (strict)
1. ## SUMMARY — one paragraph describing scope, triggers, and high-level results.
2. ## SCOPE — audit mode, initiatives, artefacts consulted.
3. ## BASELINE — dependency graph and recent-change context.
4. ## INVARIANT_STATUS — table or checklist per invariant with citations.
5. ## MODULE_ANALYSIS — per-module findings (API surface, dependencies, docs, telemetry).
6. ## CROSS_CUTTING — theme-by-theme observations and evidence.
7. ## FINDINGS — grouped by 🔴/⚠️/💡 with recommended actions.
8. ## FOLLOW_UP — explicit task list with owners, priorities, and links.
9. ## APPENDIX — optional tool outputs or additional references.
==============================================================================
PROJECT STANDARDS (Test Engine)
- Respect invariants and policies in `docs/architecture.md`, `docs/conventions.md`, and `CODING_STYLE.md`.
- Use `engine::Result<T, Error>` for recoverable failures; document ownership semantics.
- Keep roadmap/task/docs artefacts synchronised with audit findings.
```

## Tools & Automation

### Static Analysis
```
# Check for circular dependencies
cmake --graphviz=deps.dot .
dot -Tpng deps.dot -o deps.png

# Count lines of code
cloc engine/ --by-file

# Identify complex functions
lizard engine/ | sort -nr -k 6 | head
```

### Test & Coverage
```
# Generate coverage report (if configured)
cmake --preset linux-gcc-debug-coverage
cmake --build --preset linux-gcc-debug-coverage
ctest --preset linux-gcc-debug-coverage
lcov --capture --directory . --output-file coverage.info
genhtml coverage.info --output-directory coverage_html
```

### Documentation Validation
```
python scripts/validate_docs.py
```

### Dependency Graph Inspection
```
cmake --graphviz=full.dot -B build
rg "engine_" full.dot > engine_deps.dot
dot -Tpng engine_deps.dot -o engine_deps.png
```

## Example Audit Findings

### Finding: Resource Lifetime Inconsistency
- **Severity:** 🟡 Medium
- **Modules:** Animation, Physics
- **Issue:** Animation clips and physics bodies rely on raw IDs instead of generational handles, risking stale references.
- **Evidence:**
  - `engine/animation/include/engine/animation/api.hpp:45` — raw `uint32_t` identifiers.
  - `engine/physics/include/engine/physics/api.hpp:67` — body indices without generation.
- **Recommendation:** introduce `ClipHandle`/`BodyHandle`, migrate storage, add validation tests, and document lifecycle rules.
- **Task:** create `T-0125-animation-physics-handle-migration.md`.

### Finding: CUDA Hard Dependency
- **Severity:** 🔴 High
- **Modules:** Compute, Runtime
- **Issue:** CUDA is mandatory even for CPU-only builds, blocking lean deployments.
- **Evidence:**
  - `engine/compute/CMakeLists.txt:15` — unconditional `find_package(CUDA)`.
  - `engine/runtime/CMakeLists.txt:23` — always links `engine_compute_cuda`.
- **Recommendation:** land DC-002, add `ENGINE_ENABLE_CUDA`, introduce dispatcher abstraction, and add CPU-only preset.

### Finding: Missing Integration Tests
- **Severity:** 🟡 Medium
- **Modules:** Runtime, Rendering
- **Issue:** No end-to-end runtime → frame-graph → backend submission coverage.
- **Evidence:** `engine/tests/integration/` lacks executable targets.
- **Recommendation:** build integration harness, add runtime submission scenarios, document patterns, and track under `TI-001`.

## Post-Audit Actions
1. **Triage** — create roadmap/task updates for 🔴 issues; file backlog entries for ⚠️; document 💡 suggestions.
2. **Update Artefacts** — synchronise module READMEs, module roadmaps, central roadmap, and relevant specs/prints with findings.
3. **Communicate** — brief module owners, share in architecture review, and log outcomes in `docs/prints/` if required.
4. **Schedule Follow-Up** — set next audit cadence, monitor improvement plan progress, and capture telemetry trends validating fixes.
