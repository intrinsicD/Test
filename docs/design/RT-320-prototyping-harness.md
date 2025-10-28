# RT-320 Prototyping Harness Design (Preview)

**Status**: Draft implementation in progress – headless harness scaffold delivered in Python to unblock AI-004 integration.

**Related Tasks**: `RT-320`, `AI-004`, `DC-040`, `RE-610`, `TL-210`, `AS-330`

**Related ADRs**: [`ADR-0007-ai-004-configuration-schema`](../specs/ADR-0007-ai-004-configuration-schema.md)

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
2. `PrototypeHarness` validates dataset references and prepares runtime execution parameters.
3. `PrototypeHarness.run_headless()` acquires an `EngineRuntimeHandle` from the injected factory, initializes the runtime, executes a fixed timestep loop, and records summary telemetry (frame count, dataset slug, dispatch order placeholder).
4. Results are returned to the caller for reporting or persisted for benchmark automation.

### Data Model

- **Configuration**: `Ai004Configuration` composed of dataset manifests, rendering presets, runtime overrides, benchmark scenarios, and telemetry options.
- **Harness Options**: `HarnessExecutionOptions` (new) capturing frame count, timestep, and dry-run semantics.
- **Execution Result**: `HarnessRunSummary` capturing dataset slug, rendered preset, frames executed, and placeholder telemetry fields ready for integration with diagnostics exports.

### Error Handling

- Schema violations raise `ConfigurationSchemaError` before harness construction.
- Missing dataset slugs or unsupported runtime parameters raise `PrototypeHarnessError` with actionable context (slug, field name).
- Runtime load failures bubble up as `EngineLibraryNotFound` to surface missing shared libraries.
- Execution ensures runtime shutdown via context manager semantics even if tick raises.

### Logging & Telemetry

- Harness prints structured summaries (slug, preset, frames, dt) to stdout for CI consumption.
- Telemetry capture hooks currently record dispatch order samples; future work will integrate Tracy + diagnostics bridges.

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
| Native C++ harness sample | Runtime | Mirror Python scaffold in `engine/runtime/samples/prototype_harness.cpp`. |
| Dataset asset preloading (`AS-330`) | Assets | Resolve manifest entries into asset streaming requests pre-run. |

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

