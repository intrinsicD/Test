# AI-004 Configuration Schema Specification (Draft)

**Status**: ✅ Active – dataset, rendering, runtime, benchmark, and telemetry sections captured via shared validator and mirrored type hints

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

The operational workflow for applying the schema across runtime, tools, and benchmarking now lives in the
[`AI-004 Prototyping Playbook`](AI-004-prototyping-playbook.md); keep both records synchronized when fields evolve.

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
| `source.mesh_sha256` | `string` | ✅ *(v2+)* | 64-character hexadecimal SHA-256 digest of the source mesh. Required starting with schema version 2. |
| `source.mesh_size_bytes` | `integer` | ✅ *(v2+)* | Non-negative file size recorded during packaging. Required starting with schema version 2. |
| `outputs.mesh` | `string` | ✅ | Produced mesh asset registered with runtime/rendering harnesses. |
| `outputs.mesh_sha256` | `string` | ✅ *(v2+)* | SHA-256 digest of the remeshed output (schema version ≥2). |
| `outputs.mesh_size_bytes` | `integer` | ✅ *(v2+)* | File size of the remeshed output (schema version ≥2). |
| `remeshing.mode` | `string` | ✅ | Remeshing policy: `uniform`, `feature_preserving`, or `adaptive`. |
| `remeshing.targets.*` | numeric | ❌ | Optional edge/error targets; fields omitted when unset. |
| `feature_preservation.*` | boolean/float | ✅ | Flags controlling boundary/feature locking and threshold angles. |
| `metrics.input.*` | numeric | ✅ | Input mesh statistics (vertex/face counts, edge length distribution). |
| `metrics.output.*` | numeric | ✅ | Output mesh statistics mirroring input structure. |
| `provenance.summary` | `string` | ✅ *(v2+)* | Concise description of dataset origin and intent. |
| `provenance.source` | `string` | ❌ | Generator, capture pipeline, or upstream repository that produced the dataset. |
| `provenance.attribution` | `string` | ❌ | Attribution or contact to surface in tooling summaries. |
| `provenance.license.name` | `string` | ✅ *(v2+)* | License identifier or label (prefer SPDX identifiers). |
| `provenance.license.url` | `string` | ❌ | Link to the full license text. |
| `provenance.license.notes` | `string` | ❌ | Additional licensing clarifications. |
| `provenance.links[*].label` | `string` | ❌ | Optional label for a provenance documentation link. |
| `provenance.links[*].url` | `string` | ❌ | Link to packaging notes, upstream repositories, or audits. |
| `parameterization.*` | mixed | ❌ | Present when UV generation or reuse is requested; includes mode, density, chart stats. |
| `statistics.iterations` | `integer` | ✅ | Total solver iterations executed by the remesher. |
| `statistics.splits` | `integer` | ❌ | Optional count of split operations applied during remeshing. |
| `statistics.collapses` | `integer` | ❌ | Optional count of collapse operations applied during remeshing. |
| `statistics.duration_ms` | `float` | ❌ | Optional wall-clock execution time in milliseconds. |
| `statistics.max_error` | `float` | ✅ | Maximum edge-length error recorded during the solve. |
| `statistics.min_edge_length` | `float` | ✅ | Minimum edge length observed in the output mesh. |
| `statistics.max_edge_length` | `float` | ✅ | Maximum edge length observed in the output mesh. |
| `statistics.max_surface_deviation` | `float` | ✅ | Maximum sampled surface deviation between input and output meshes. |
| `statistics.mean_surface_deviation` | `float` | ✅ | Mean sampled surface deviation. |
| `statistics.rms_surface_deviation` | `float` | ✅ | Root-mean-square surface deviation. |
| `statistics.triangles` | `integer` | ❌ | Optional output triangle count captured by telemetry. |
| `statistics.triangle_quality.min/mean/max` | `float` | ❌ | Optional triangle quality distribution (0–1 range). |

All numeric values are emitted with fixed precision to simplify regression testing
and diffing. Paths are rendered using POSIX separators so manifests remain
portable across platforms.

Dataset identifiers must be unique within a manifest. Validators reject
duplicate slugs so downstream tooling can assume one-to-one mappings when
resolving dataset references from runtime or benchmark configuration sections.

Starting with schema version 2, manifests must also publish a `provenance`
block capturing licensing and attribution so runtime summaries and tooling can
surface source/permission context alongside telemetry output.

### Example

```yaml
datasets:
  - id: remesh-sample
    schema:
      id: ai-004.dataset
      version: 2
    kind: geometry.remesh
    tags: [geometry, remesh]
    source:
      generator: geometry_remesh
      mesh: assets/bunny.obj
      mesh_sha256: 0f6d4e8ab153c90db2e33c2a7b1b8d51b26cc5fb33bb1b7c92b7323ce6c9a4a9
      mesh_size_bytes: 348160
    outputs:
      mesh: assets/bunny_remeshed.obj
      mesh_sha256: 12f4d6c97d6a87c4d83219c3ce62837c0f4b0c5fb8b51e0a2d53b6a9d7205c46
      mesh_size_bytes: 198752
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
    provenance:
      summary: Procedurally generated remeshing dataset for regression coverage.
      source: geometry_remesh CLI using sample assets
      attribution: AI-004 Dataset Team
      license:
        name: CC0 1.0 Universal
        url: https://creativecommons.org/publicdomain/zero/1.0/
      links:
        - label: Packaging notes
          url: docs/examples/README.md#ai004_samplejson
    statistics:
      iterations: 12
      splits: 3
      collapses: 2
      duration_ms: 4.5
      max_error: 0.0015
      max_surface_deviation: 0.0150
      mean_surface_deviation: 0.0100
      rms_surface_deviation: 0.0120
      triangles: 34560
      triangle_quality:
        min: 0.7200
        mean: 0.8600
        max: 0.9400
```

The `statistics` block records both the edge-length deviation budget (`max_error`) and
approximate Hausdorff metrics gathered from bidirectional sampling between the
input mesh and remeshed output (`max_surface_deviation`, `mean_surface_deviation`,
`rms_surface_deviation`). Optional counters (`splits`, `collapses`, `triangles`),
runtime duration (`duration_ms`), and quality ranges (`triangle_quality`) expose
operational telemetry that the prototyping harness surfaces for downstream tools.

Packaging workflows should consume these manifests via
`python -m scripts.datasets.ingest_dataset` so dataset caches include
checksummed file metadata and reproducible directory layouts before the runtime
harness (`RT-320`) stages assets.

> **Schema Evolution:** Dataset schema version 2 introduces explicit
> `mesh_sha256` and `mesh_size_bytes` entries for both the source input and
> remeshed outputs alongside mandatory provenance/licensing blocks. The
> ingestion tooling rejects manifests whose recorded values do not match
> on-disk assets and surfaces provenance directly in summaries, guaranteeing
> reproducible packaging while keeping licensing obligations visible to
> downstream consumers.

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
predictably. Static analyzers and editors consume the mirrored type stubs in
[`python/engine3g/config_schema.pyi`](../../python/engine3g/config_schema.pyi)
so downstream tooling can reason about schema-backed data without importing the
runtime loader.

Native runtime consumers share the same contract via
[`engine/runtime/config_schema.hpp`](../../engine/runtime/include/engine/runtime/config_schema.hpp).
`engine::runtime::config::load_dataset_manifest()` and
`engine::runtime::config::load_configuration()` parse YAML/JSON manifests with
`yaml-cpp` and return `RuntimeResult<T>` values so harness integrations surface
`RuntimeError::configuration_*` codes instead of throwing. The behaviour is
covered by [`engine/runtime/tests/test_config_schema.cpp`](../../engine/runtime/tests/test_config_schema.cpp).

Cross-referencing is also enforced: runtime configuration and benchmark
scenarios must reference dataset slugs declared in the manifest, and validators
raise descriptive errors when a slug is missing or duplicated.

`python -m scripts.prototyping.run_prototype_harness --require-schema` toggles
strict enforcement for individual runs while the `ENGINE_AI004_SCHEMA_V1`
feature flag enables rollout-wide validation. Use the CLI alongside
`scripts.validate_ai004_config` during migration to prevent legacy manifests
from bypassing schema headers.

Callers should surface `ConfigurationSchemaError` messages directly to users so
schema violations remain actionable during asset packaging or prototyping
workflows.

---

_This record remains the coordination hub for `DC-040`; extend it alongside future schema revisions and module sign-off notes._
