# AI-004 Configuration Schema Specification (Draft)

**Status**: Draft – awaiting schema consolidation per `DC-040`

**Related Tasks**: `AI-004`, `DC-040`, `RE-610`, `RT-320`, `TL-210`, `AS-330`, `CC-310`

**Related ADR**: [`ADR-0007-ai-004-configuration-schema`](../specs/ADR-0007-ai-004-configuration-schema.md)

---

## Purpose

Capture the shared configuration contract required by `AI-004` deliverables so rendering presets, runtime harness parameters, sandbox UI widgets, dataset manifests, and benchmark automation agree on structure, validation rules, and ownership. This document will evolve alongside the ADR and task card to provide canonical field definitions, examples, and migration notes.

## Next Steps

1. Inventory existing manifests from runtime, rendering presets, sandbox UI prototypes, and dataset packages.
2. Draft schema sections covering:
   - `datasets`: identifiers, provenance, licensing metadata, scale units.
   - `rendering`: preset identifiers, debug overlay toggles, backend requirements.
   - `runtime`: scene graph selections, simulation cadence, hot reload policies.
   - `benchmarks`: scenario definitions, regression thresholds, telemetry exports.
   - `telemetry`: metric selection, sampling cadence, retention policies.
3. Validate schema against representative configurations and record findings here.
4. Update this document with field tables, versioning strategy, and migration checklist prior to kickoff review.

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
```

Downstream consumers (runtime harness, sandbox UI, benchmark orchestrator) must
reject entries whose `schema.id`/`schema.version` do not match the supported
release so misconfigured manifests fail fast.

---

_This draft exists to unblock coordination and will be fleshed out as part of `DC-040`._
