# RT-320 Prototyping Harness Design (Preview)

**Status**: Draft implementation in progress – Python scaffold plus native C++ sample (`runtime_prototype_harness`) available for
dry-run validation and CI coverage.

**Related Tasks**: `RT-320`, `AI-004`, `DC-040`, `RE-610`, `TL-210`, `AS-330`

**Related ADRs**: [`ADR-0007-ai-004-configuration-schema`](../specs/ADR-0007-ai-004-configuration-schema.md)

---

## Priority Context

RT-320 anchors the Phase 1 AI-004 kickoff plan: the roadmap commits to a 2026-02-12 smoke test ahead of TL-210 sandbox wiring and AS-330 dataset validation. The harness must therefore project schema versions, telemetry descriptors, and dataset provenance so neighbouring workstreams can assert compatibility without reverse-engineering manifests. Failing to deliver those summaries blocks TL-210’s UI widgets from enabling preset/dataset selection, prevents AS-330 from signing off its package validation scripts, and leaves CC-310’s comparative automation without telemetry inputs to detect regressions.

---

## Purpose

Provide an immediately usable prototyping harness that consumes the shared AI-004 configuration schema, boots the engine runtime, and executes deterministic headless simulations suitable for benchmarking and automation. The harness must offer:

- Schema-backed configuration parsing shared with tools and dataset packaging.
- Deterministic runtime execution for headless benchmarking.
- Extensibility hooks for interactive tooling (e.g., TL-210 sandbox UI).
- Telemetry capture plumbing aligned with `CC-001`.

This design captures the initial Python-based scaffold that exercises the runtime library via the existing ctypes bindings while C++-native integrations are developed. A canonical sample configuration (`docs/examples/ai004_sample.json`) accompanies the scaffold so contributors can validate the harness against the `assets/datasets/remesh_sample` package without drafting manifests from scratch.

---

## Architecture Overview

```text
AI-004 Configuration (YAML/JSON)
        │
        ▼
python/engine3g/config_schema.load_configuration()
        │  (validated Ai004Configuration dataclass)
        ▼
PrototypeHarness (python/engine3g/prototype_harness)
        │
        ├─ Dataset selection + validation (DatasetManifest)
        ├─ Runtime parameter resolution (timestep, camera, hot reload flags)
        ├─ Runtime factory (engine3g.loader.load_runtime)
        └─ Execution pipeline (headless loop + telemetry capture stubs)
        │
        ▼
EngineRuntimeHandle (ctypes wrapper)
        │
        └─ Native RuntimeHost (C++)
```

### Control Flow

1. CLI (`scripts/prototyping/run_prototype_harness.py`) parses arguments and loads configuration via `load_configuration`.
   - Use `--list-case-studies` to surface bundled presets (from `assets/datasets/case_studies/`) and optionally
     `--case-studies-json <path>` to export metadata for TL-210's sandbox selectors without executing the runtime loop.
2. `PrototypeHarness` validates dataset references and prepares runtime execution parameters.
3. `PrototypeHarness.run_headless()` acquires an `EngineRuntimeHandle` from the injected factory, initializes the runtime, executes a fixed timestep loop, and records summary telemetry (frame count, dataset slug, dispatch order placeholder).
4. Results are returned to the caller for reporting or persisted for benchmark automation.

### Data Model

- **Configuration**: `Ai004Configuration` composed of dataset manifests, rendering presets, runtime overrides, benchmark scenarios, and telemetry options. The `PrototypeHarness.describe_configuration()` helper now projects this into a structured summary that includes schema versions for datasets/runtime/rendering plus telemetry outputs/metrics so UI clients can verify compatibility at a glance. Dataset assets now surface resolved paths, checksum comparisons, and verification state so tooling can highlight missing or stale packages without rerunning ingestion scripts.
- **Harness Options**: `HarnessExecutionOptions` (new) capturing frame count, timestep, and dry-run semantics.
- **Configuration Summary**: `HarnessConfigurationSummary` materialises dataset provenance (tags, statistics, feature-preservation flags, parameterisation metrics, checksums), runtime defaults, and telemetry descriptors for consumption by tools automation. Statistics now include remeshing operation counts, execution duration, and triangle-quality ranges so downstream tooling mirrors dataset manifests without reparsing them.
- **Execution Result**: `HarnessRunSummary` capturing dataset slug, rendered preset, frames executed, telemetry output targets, and dispatch telemetry ready for integration with diagnostics exports.

### Error Handling

- Schema violations raise `ConfigurationSchemaError` before harness construction.
- Missing dataset slugs or unsupported runtime parameters raise `PrototypeHarnessError` with actionable context (slug, field name).
- Runtime load failures bubble up as `EngineLibraryNotFound` to surface missing shared libraries.
- Execution ensures runtime shutdown via context manager semantics even if tick raises.

### Logging & Telemetry

- Harness prints structured summaries (slug, preset, frames, dt, and average tick time) to stdout for CI consumption.
- Execution summaries now include the runtime's rolling `average_tick_ms` diagnostics, per-kernel dispatch telemetry (execution order + durations in milliseconds), and resolved telemetry output targets so benchmark automation can flag timing regressions and validate export paths without parsing native logs. The native `runtime_prototype_harness` sample mirrors this behaviour by serialising dispatch order, millisecond durations, and telemetry outputs alongside the JSON summary consumed by CI.
- Telemetry capture hooks record dispatch order samples and kernel durations; future work will integrate Tracy + diagnostics bridges. The configuration summary additionally surfaces telemetry schema version, outputs, metrics, and sampling cadence so downstream tooling can validate expectations without parsing manifests directly.

### Extensibility

- Runtime factory injection enables unit testing with mock runtimes and future substitution of native C++ harness components.
- Result dataclass provides stable contract for benchmark orchestration (`scripts/benchmarks` follow-ups).
- CLI exposes `--dry-run` for schema validation without runtime startup, enabling dataset/package CI checks.

---

## Outstanding Work

| Item | Owner | Notes |
|------|-------|-------|
| Integrate rendering preset wiring (`RE-610`) | Rendering | Map `RenderingConfig` into runtime initialization. |
| Surface telemetry outputs (`CC-310`) | Performance | Populate `HarnessRunSummary.telemetry_paths` once runtime exports metrics. |
| Interactive mode bridge (`TL-210`) | Tools | Embed ImGui controls and expose hot-reload toggles. |
| Native C++ harness sample | Runtime | ✅ `runtime_prototype_harness` CLI sample mirrors Python dry-run flow (2026-02-05). |

**2026-02-08** — Harness construction now validates dataset asset existence, file sizes, and sha256 hashes. Configuration summaries expose per-asset integrity status for TL-210 selectors and AS-330 packaging checks.

---

## Risks & Mitigations

| Risk | Impact | Mitigation |
|------|--------|-----------|
| Python harness diverges from future C++ entry point | Medium | Keep business logic in reusable dataclasses; mirror behaviour in C++ once resource providers land. |
| Runtime library unavailable on contributor machines | Medium | Provide clear error messages and `--dry-run` validation path. |
| Configuration drift across modules | High | Continue sourcing schema from shared loader (`config_schema.py`) and update tasks/docs concurrently. |

---

## References

- [`python/engine3g/config_schema.py`](../../python/engine3g/config_schema.py)
- [`python/engine3g/loader.py`](../../python/engine3g/loader.py)
- [`docs/archive/backlog/legacy/tasks/RT-320-runtime-prototyping-harness.md`](../archive/backlog/legacy/tasks/RT-320-runtime-prototyping-harness.md)
- [`scripts/validate_ai004_config.py`](../../scripts/validate_ai004_config.py)

