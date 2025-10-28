# AI-004 Configuration Schema Specification (Draft)

**Status**: In review – dataset, rendering, runtime, benchmark, and telemetry sections captured via shared validator

**Related Tasks**: `AI-004`, `DC-040`, `RE-610`, `RT-320`, `TL-210`, `AS-330`, `CC-310`

**Related ADR**: [`ADR-0007-ai-004-configuration-schema`](../specs/ADR-0007-ai-004-configuration-schema.md)

---

## Purpose

Capture the shared configuration contract required by `AI-004` deliverables so rendering presets, runtime harness parameters, sandbox UI widgets, dataset manifests, and benchmark automation agree on structure, validation rules, and ownership. This document will evolve alongside the ADR and task card to provide canonical field definitions, examples, and migration notes.

## Next Steps

1. Inventory existing manifests from runtime, rendering presets, sandbox UI prototypes, and dataset packages. ✅ Dataset manifests captured via `geometry_remesh` output and regression tests in `python/tests/test_config_schema.py`.
2. Draft schema sections covering:
   - `datasets`: identifiers, provenance, licensing metadata, scale units.
   - `rendering`: preset identifiers, debug overlay toggles, backend requirements.
   - `runtime`: scene graph selections, simulation cadence, hot reload policies.
   - `benchmarks`: scenario definitions, regression thresholds, telemetry exports.
   - `telemetry`: metric selection, sampling cadence, retention policies.
3. Validate schema against representative configurations and record findings here. ✅ Dataset manifests validated with strict slug rules, telemetry fields, and optional parameterisation metadata.
4. Update this document with field tables, versioning strategy, and migration checklist prior to kickoff review.

### Validation Tooling

- `python -m scripts.validate_ai004_config --dataset <manifest> --config <configuration>` validates manifests with the shared
  Python loader so pipelines can fail fast when schema drift is detected. ✅ Script added alongside automated tests to unblock
  CI wiring and local iteration.
- `pytest python/tests/test_prototype_harness.py::test_cli_integration_with_sample_assets` executes the harness CLI against
  repository sample manifests, providing a CI-friendly smoke test for schema-regressed configurations.

---

## Dataset Section (v1 Draft)

| Field | Type | Required | Description |
|-------|------|----------|-------------|
| `schema.id` | `string` | ✅ | Canonical identifier for the schema component. For datasets use `ai-004.dataset`. |
| `schema.version` | `integer` | ✅ | Schema revision. Initial release uses `1`. |
| `id` | `string` | ✅ | Stable slug (lowercase + hyphen) referenced by runtime harness and benchmark scenarios. |
| `kind` | `string` | ✅ | Source of the dataset (`geometry.remesh`, `animation.capture`, etc.). |
| `tags` | `array<string>` | ✅ | Classification tags for filtering/UX (e.g., `geometry`, `remesh`). |
| `job_label` | `string` | ❌ | Optional human-readable label surfaced in telemetry dashboards. |
| `source.generator` | `string` | ✅ | Tool or pipeline that produced the dataset entry (for example `geometry_remesh`). |
| `source.mesh` | `string` | ✅ | Path or URI to the source asset consumed by the generator. |
| `outputs.mesh` | `string` | ✅ | Produced mesh asset registered with runtime/rendering harnesses. |
| `remeshing.mode` | `string` | ✅ | Remeshing policy: `uniform`, `feature_preserving`, or `adaptive`. |
| `remeshing.targets.*` | numeric | ❌ | Optional edge/error targets; fields omitted when unset. |
| `feature_preservation.*` | boolean/float | ✅ | Flags controlling boundary/feature locking and threshold angles. |
| `metrics.input.*` | numeric | ✅ | Input mesh statistics (vertex/face counts, edge length distribution). |
| `metrics.output.*` | numeric | ✅ | Output mesh statistics mirroring input structure. |
| `parameterization.*` | mixed | ❌ | Present when UV generation or reuse is requested; includes mode, density, chart stats. |
| `statistics.*` | numeric | ✅ | Iteration counts and error envelopes reported by remesher telemetry. |

All numeric values are emitted with fixed precision to simplify regression testing
and diffing. Paths are rendered using POSIX separators so manifests remain
portable across platforms.

### Example

```yaml
datasets:
  - id: remesh-sample
    schema:
      id: ai-004.dataset
      version: 1
    kind: geometry.remesh
    tags: [geometry, remesh]
    source:
      generator: geometry_remesh
      mesh: assets/bunny.obj
    outputs:
      mesh: assets/bunny_remeshed.obj
    remeshing:
      mode: uniform
      targets:
        target_edge_length: 0.2500
    feature_preservation:
      lock_boundary_edges: true
      lock_feature_edges: true
      minimum_feature_angle_degrees: 45.0000
    metrics:
      input:
        vertices: 34834
        faces: 69648
      output:
        vertices: 17290
        faces: 34560
    statistics:
      iterations: 12
      max_error: 0.0015
      max_surface_deviation: 0.0150
      mean_surface_deviation: 0.0100
      rms_surface_deviation: 0.0120
```

The `statistics` block records both the edge-length deviation budget (`max_error`) and
approximate Hausdorff metrics gathered from bidirectional sampling between the
input mesh and remeshed output (`max_surface_deviation`, `mean_surface_deviation`,
`rms_surface_deviation`).

Packaging workflows should consume these manifests via
`python -m scripts.datasets.ingest_dataset` so dataset caches include
checksummed file metadata and reproducible directory layouts before the runtime
harness (`RT-320`) stages assets.

Downstream consumers (runtime harness, sandbox UI, benchmark orchestrator) must
reject entries whose `schema.id`/`schema.version` do not match the supported
release so misconfigured manifests fail fast.

---

## Rendering Section (v1 Draft)

| Field | Type | Required | Description |
|-------|------|----------|-------------|
| `schema.id` | `string` | ✅ | Always `ai-004.rendering`. |
| `schema.version` | `integer` | ✅ | Initial release `1`. |
| `preset` | `string` | ✅ | Rendering preset identifier (e.g., `research-baseline`). |
| `options.shading_mode` | `string` | ❌ | `forward` or `deferred`; defaults to deferred. |
| `options.resolution.width` | `integer` | ❌ | Frame width in pixels; defaults to `1920`. |
| `options.resolution.height` | `integer` | ❌ | Frame height in pixels; defaults to `1080`. |
| `options.overlays.normals` | `bool` | ❌ | Enable normals debug overlay. |
| `options.overlays.uv` | `bool` | ❌ | Enable UV density overlay. |
| `options.overlays.material` | `bool` | ❌ | Enable material ID overlay. |
| `options.overlays.light_volume` | `bool` | ❌ | Enable light volume overlay. |

The rendering section mirrors `ResearchBaselineOptions` so presets can be
generated directly from configuration without bespoke glue code.

---

## Runtime Section (v1 Draft)

| Field | Type | Required | Description |
|-------|------|----------|-------------|
| `schema.id` | `string` | ✅ | Always `ai-004.runtime`. |
| `schema.version` | `integer` | ✅ | Initial release `1`. |
| `dataset` | `string` | ❌ | Dataset slug selected for the session. |
| `scene.manifest` | `string` | ❌ | Path to runtime scene manifest. |
| `scene.entry_point` | `string` | ❌ | Optional entry point identifier. |
| `camera.mode` | `string` | ❌ | `orbit`, `fly`, or `fixed`. |
| `camera.position` | `vec3` | ❌ | Starting camera position. |
| `camera.target` | `vec3` | ❌ | Look-at target; orbit mode uses it as pivot. |
| `simulation.timestep_seconds` | `float` | ❌ | Fixed simulation timestep; defaults to engine configuration. |
| `simulation.max_substeps` | `integer` | ❌ | Max physics substeps per frame. |
| `hot_reload.enabled` | `bool` | ❌ | Toggle asset/script hot reload. |
| `hot_reload.watch_interval_seconds` | `float` | ❌ | File watcher polling cadence. |

---

## Benchmark Section (v1 Draft)

| Field | Type | Required | Description |
|-------|------|----------|-------------|
| `schema.id` | `string` | ✅ | Always `ai-004.benchmarks`. |
| `schema.version` | `integer` | ✅ | Initial release `1`. |
| `scenarios[].id` | `string` | ✅ | Slug for cross-referencing scenario. |
| `scenarios[].name` | `string` | ✅ | Human readable scenario label. |
| `scenarios[].dataset` | `string` | ❌ | Dataset slug when scenario consumes assets. |
| `scenarios[].rendering_preset` | `string` | ❌ | Rendering preset to activate. |
| `scenarios[].runtime_profile` | `string` | ❌ | Runtime profile identifier. |
| `scenarios[].engine.command` | `array<string>` | ❌ | Command tokens for engine execution; optional for dry runs. |
| `scenarios[].engine.output` | `string` | ✅ | Path template for engine metrics output. |
| `scenarios[].reference.command` | `array<string>` | ❌ | Command tokens for reference implementation. |
| `scenarios[].reference.output` | `string` | ✅ | Path template for reference metrics output. |
| `scenarios[].metrics[].name` | `string` | ✅ | Metric identifier (`fps`, `frame_time`, etc.). |
| `scenarios[].metrics[].higher_is_better` | `bool` | ✅ | Comparison orientation for the metric. |
| `scenarios[].metrics[].threshold.type` | `string` | ✅ | `relative` (requires `max_regression`) or `absolute` (requires `max_delta`). |

Metrics reuse the comparative benchmark orchestrator semantics so schema-driven
configurations flow directly into `run_comparative_benchmarks.py` without
translation layers.

---

## Telemetry Section (v1 Draft)

| Field | Type | Required | Description |
|-------|------|----------|-------------|
| `schema.id` | `string` | ✅ | Always `ai-004.telemetry`. |
| `schema.version` | `integer` | ✅ | Initial release `1`. |
| `outputs[].type` | `string` | ❌ | `file` or `stdout`. |
| `outputs[].path` | `string` | ✅* | Required when type is `file`. |
| `metrics[].name` | `string` | ❌ | Metric identifier (e.g., `frame_time`). |
| `metrics[].statistic` | `string` | ❌ | Aggregation mode (`mean`, `median`, `p95`, `p99`, `min`, `max`). |
| `sampling.frame_interval` | `integer` | ❌ | Sample every N frames; defaults to `1`. |
| `sampling.include_debug_overlays` | `bool` | ❌ | Include overlay telemetry in captures. |

---

## Top-Level Configuration Example

```yaml
datasets:
  - ...  # Dataset entry from the section above
rendering:
  schema:
    id: ai-004.rendering
    version: 1
  preset: research-baseline
  options:
    shading_mode: forward
    resolution:
      width: 1280
      height: 720
    overlays:
      normals: true
runtime:
  schema:
    id: ai-004.runtime
    version: 1
  dataset: remesh-sample
  scene:
    manifest: scenes/remesh.scene
    entry_point: main
  camera:
    mode: orbit
    position: [0.0, 1.0, 2.0]
    target: [0.0, 0.5, 0.0]
  simulation:
    timestep_seconds: 0.0166667
    max_substeps: 4
  hot_reload:
    enabled: true
    watch_interval_seconds: 0.5
benchmarks:
  schema:
    id: ai-004.benchmarks
    version: 1
  scenarios:
    - id: remesh-baseline
      name: Remesh Baseline
      dataset: remesh-sample
      rendering_preset: research-baseline
      engine:
        command: ["python", "engine.py", "{output_path}"]
        output: "{output_dir}/{scenario}_engine.json"
      reference:
        command: ["python", "reference.py", "{output_path}"]
        output: "{output_dir}/{scenario}_reference.json"
      metrics:
        - name: fps
          higher_is_better: true
          threshold:
            type: relative
            max_regression: 0.05
telemetry:
  schema:
    id: ai-004.telemetry
    version: 1
  outputs:
    - type: file
      path: telemetry/{scenario}.json
  metrics:
    - name: frame_time
      statistic: mean
  sampling:
    frame_interval: 2
    include_debug_overlays: true
```

---

## Validation Helpers

Dataset manifests now have an executable reference validator under
[`python/engine3g/config_schema.py`](../../python/engine3g/config_schema.py).
`load_configuration()` and `load_dataset_manifest()` accept YAML or JSON
documents. Validation enforces slug requirements (`[a-z0-9-]+`), rendering
preset invariants, runtime camera/simulation bounds, benchmark threshold
semantics, and telemetry output declarations. Parsed manifests materialise as
dataclasses for downstream tooling. Regression tests under
`python/tests/test_config_schema.py` exercise dataset-only manifests alongside
full multi-section configurations to guarantee optional sections behave
predictably.

`python -m scripts.prototyping.run_prototype_harness --require-schema` toggles
strict enforcement for individual runs while the `ENGINE_AI004_SCHEMA_V1`
feature flag enables rollout-wide validation. Use the CLI alongside
`scripts.validate_ai004_config` during migration to prevent legacy manifests
from bypassing schema headers.

Callers should surface `ConfigurationSchemaError` messages directly to users so
schema violations remain actionable during asset packaging or prototyping
workflows.

---

_This draft exists to unblock coordination and will be fleshed out as part of `DC-040`._
