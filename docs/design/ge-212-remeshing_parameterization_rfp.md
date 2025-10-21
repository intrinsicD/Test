# GE-212 — Remeshing & Parameterisation RFP

_Last Updated: 2025-03-26_

## Summary
The geometry module requires a roadmap-quality specification for surface remeshing
and parameterisation so that downstream animation, rendering, and tooling teams can
plan dependencies with confidence. This request for proposal (RFP) defines
requirements, constraints, and evaluation criteria for introducing feature-complete
surface remeshing and atlas generation pipelines that integrate with existing
`SurfaceMesh` utilities, IO round-tripping guarantees, and runtime deformation
workflows. The output of `GE-212` is a design alignment document; implementation will
follow in subsequent initiatives once staffing and sequencing are approved.

## Problem Statement
Current geometry capabilities focus on mesh inspection, deformation, and IO
validation. We lack:

- Authoritative requirements for surface remeshing (uniform, adaptive, or feature
  preserving) that keep topology and attribute fidelity compatible with
  animation and physics pipelines.
- Guidance on UV parameterisation workflows needed by rendering, tooling, and asset
  baking. Texture artists cannot rely on deterministic atlas generation without a
  shared specification.
- Integration notes describing how remeshing interacts with asset caches, async
  streaming, and hot reload infrastructure.

Without a scoped plan, geometry upgrades risk conflicting implementations and
incomplete observability. This RFP captures the baseline expectations to unblock
subsequent execution tasks (`GE-221+`) and telemetry instrumentation (`GE-220`).

## Goals
1. Document functional and non-functional requirements for surface remeshing and UV
   parameterisation in the Test Engine geometry module.
2. Identify interoperability constraints with IO round-tripping, runtime deformation,
   and rendering pipelines.
3. Define evaluation metrics, benchmarks, and test coverage expectations that will
   accompany future implementations.
4. Provide phased execution guidance and dependency mapping so roadmap planners can
   schedule follow-on work.

## Non-Goals
- Shipping production remeshing code or integrating specific algorithms.
- Redesigning `SurfaceMesh` data structures beyond clarifying required extensions
  (e.g., attribute storage, boundary flags).
- Specifying GPU implementations; focus remains on CPU reference paths with hooks
  for future acceleration initiatives.
- Covering volumetric (tetrahedral) remeshing or parameterisation for now; scope is
  triangle meshes with manifold or mixed-manifold connectivity.

## Stakeholders & Dependencies
- **Geometry module owners** – implement remeshing/parameterisation kernels and tests.
- **Assets module** – coordinates async streaming, hot reload, and cache persistence
  for derived meshes or UV atlases.
- **Rendering module** – consumes parameterisation output for material baking and
  texture sampling; requires compatibility with frame-graph resource descriptors.
- **Animation module** – expects remeshed surfaces to preserve skinning weights and
  rig bindings without invalidating runtime deformation paths.
- **Tools module** – surfaces diagnostics/preview pipelines; consumes telemetry for
  remeshing jobs.
- **IO module** – maintains round-trip fidelity for new mesh representations and
  ensures signature database coverage for generated assets.

Dependencies:
- `RT-006` IO signature hardening to register new asset variants.
- `AI-002` async streaming telemetry for tracking background remeshing jobs.
- `CC-001` telemetry schema for reporting job metrics.
- Potential coordination with future compute initiatives (`CO-170+`) if GPU
  acceleration enters scope.

## Functional Requirements
1. **Surface Remeshing Modes**
   - *Uniform remeshing* with target edge length (absolute and relative to average).
   - *Feature-preserving remeshing* respecting sharp edges, boundaries, and crease
     annotations supplied by assets or derived from curvature analysis.
   - *Adaptive remeshing* driven by error metrics (e.g., Hausdorff distance, normal
     deviation) configurable per asset.
   - Support for preserving or duplicating per-vertex/face attributes (normals,
     tangents, UVs, colors, skinning weights).

2. **UV Parameterisation**
   - Generate consistent chart atlases with configurable stretch minimisation
     strategy (e.g., LSCM, ABF++, or stretch-aware variants).
   - Provide seam marking and island packing with texel density controls.
   - Support optional atlas reuse for existing UV sets while generating additional
     channels without overwriting input data.

3. **Pipeline Integration**
   - Remeshing outputs must integrate with `SurfaceMesh` and maintain compatibility
     with geometry IO round-trip guarantees defined in
     [`ADR-0005`](../specs/ADR-0005-geometry-io-roundtrip.md).
   - Parameterisation metadata must feed rendering material baking workflows and be
     consumable by runtime deformation pipelines without breaking skinning caches.
   - Async operations must hook into the assets module request lifecycle (pending →
     loading → ready) to reuse telemetry and cancellation semantics delivered in
     `AI-002`.

4. **User Tooling**
   - Provide CLI/automation entry points for offline remeshing/parameterisation jobs
     (e.g., `geometry_remesh`), outputting diagnostics and telemetry snapshots.
   - Publish authoring documentation describing configuration parameters, supported
     asset types, and failure diagnostics.

## Non-Functional Requirements
- **Determinism**: identical inputs (mesh, seed, configuration) must yield identical
  outputs across supported platforms/presets. Provide explicit seeding for any
  stochastic components (e.g., sampling).
- **Performance Targets**: baseline CPU implementation should process 100k triangle
  meshes within 500 ms (debug) / 150 ms (release) for uniform remeshing; atlas
  generation should complete within similar bounds or document deviations.
- **Memory Footprint**: remeshing should avoid peak memory usage exceeding 4× the
  input mesh size (counts all attribute buffers) to stay within tooling limits.
- **Robustness**: handle meshes with boundaries, poles, degenerate triangles, and
  isolated components; emit actionable errors for non-manifold cases.
- **Observability**: integrate with telemetry schema (`geometry.remesh.*`,
  `geometry.parameterise.*`) and structured logging. Capture metrics for iteration
  counts, error convergence, seam lengths, and atlas utilisation.

## Data Model & API Expectations
- Extend `SurfaceMesh` to tag boundary edges, crease flags, and attribute semantics
  required by remeshing policies. Maintain backwards compatibility for existing
  helpers by providing default values when annotations are absent.
- Introduce remeshing request structures:
  ```cpp
  struct RemeshRequest {
      SurfaceMeshHandle input;
      RemeshingMode mode;
      RemeshingTargets targets; // edge length, error bounds, feature tolerances
      AttributePolicy attribute_policy; // preserve, resample, drop per channel
      ParameterisationPolicy uv_policy; // optional, for simultaneous UV updates
      TelemetryToken telemetry;
  };
  ```
- Parameterisation results should expose chart metadata, packing transforms, island
  adjacency graphs, and distortion metrics consumable by rendering and tools.
- APIs must return `engine::Result<RemeshOutput, RemeshError>` following error
  handling conventions established by `DC-004`.

## Algorithmic Options (for vendor proposals)
- **Remeshing**:
  - Isotropic remeshing via edge split-collapse and tangential smoothing (e.g.,
    Botsch & Kobbelt 2004) with feature preservation.
  - Anisotropic remeshing guided by principal curvature tensors (feature
    alignment, parameter-driven aspect ratios).
  - Delaunay-based approaches (e.g., Restricted Delaunay Triangulation) for
    improved quality when point clouds are available.
- **Parameterisation**:
  - Least Squares Conformal Maps (LSCM) or ABF++ for conformal parameterisations.
  - Stretch-minimising methods (e.g., Spectral Conformal Parameterisation) for
    texture baking.
  - Seam selection heuristics to minimise distortion and cut lengths.

Vendors must detail numerical stability strategies, boundary handling, and how they
preserve attribute fidelity (normals, tangents, weights) across remeshing.

## Validation & Testing Strategy
- **Unit Tests**: cover edge length targets, attribute preservation, error metrics,
  and UV atlas invariants (no inverted charts, seam continuity).
- **Property Tests**: fuzz small meshes to ensure operations terminate and maintain
  manifoldness, leveraging IO signature harnesses once `RT-006` lands.
- **Golden Assets**: maintain reference meshes and atlases in
  `engine/geometry/tests/data/` with checksums validated by CI.
- **Performance Benchmarks**: extend `geometry_normals_benchmark` infrastructure or
  author new benchmarks capturing runtime for representative meshes (10k–1M
  triangles) with telemetry export for historical trending.
- **Integration Tests**: ensure animation skinning remains stable after remeshing
  (joint influence remapping) and that rendering pipelines accept generated UV
  charts without regression.

## Observability & Telemetry
- Emit counters/histograms for job duration, iteration counts, Hausdorff error,
  seam length totals, atlas fill rate, and cancellation rates.
- Produce structured logs with mesh identifiers, configuration digests, and error
  reasons; integrate with diagnostics viewer to display recent runs and warnings.
- Align telemetry field names with `design/telemetry_schema.md` and plan schema
  extensions if necessary (`geometry.remesh.error.hausdorff`, etc.).

## Security & Compliance
- Validate asset paths using existing IO sanitisation routines; generated meshes
  should reside within project-controlled directories.
- Clamp configuration parameters to prevent pathological allocations (e.g., prohibit
  <1e-5 edge length targets on large meshes).
- Ensure CLI utilities default to read-only operations unless explicit overwrite
  flags are provided.

## Delivery Phases
1. **Phase 0 — Infrastructure Prep (1 sprint)**
   - Finalise remeshing/parameterisation API proposals and schema extensions to
     `SurfaceMesh`.
   - Add configuration parsing utilities and telemetry scaffolding.
2. **Phase 1 — Uniform Remeshing & Baseline UVs (1–2 sprints)**
   - Implement uniform remeshing, attribute preservation, and LSCM parameterisation.
   - Ship CLI tooling, documentation, and baseline tests.
3. **Phase 2 — Adaptive & Feature-Preserving Enhancements (2 sprints)**
   - Add curvature-driven adaptation, crease handling, and improved seam selection.
   - Integrate Hausdorff error estimation and quality metrics.
4. **Phase 3 — Advanced Workflows & Tooling (post-M4)**
   - Explore anisotropic remeshing, multi-resolution hierarchies, and integration
     with compute acceleration or GPU kernels.
   - Coordinate with Tools for editor previews and diagnostics overlays.

## Success Metrics
- Published API and data model spec reviewed by geometry, assets, and rendering
  leads; follow-up ADRs filed for any structural changes to `SurfaceMesh`.
- End-to-end prototype (Phase 1) demonstrates deterministic remeshing/UV output on
  CI hardware with telemetry instrumentation active.
- Benchmarks recorded for representative meshes and tracked via diagnostics
  tooling.
- Documentation updates land in geometry and rendering READMEs, plus runtime
  guidance for consuming parameterisation data.

## Risks & Mitigations
- **Numerical instability**: adopt robust solvers (QR/SVD with damping), add
  validation passes, and expose configuration to clamp degeneracy thresholds.
- **Attribute loss**: define explicit policies for reprojecting normals/tangents and
  rebalancing skinning weights post-remesh; include regression tests.
- **Performance regressions**: stage features behind configuration flags and profile
  using CI benchmarks before enabling by default.
- **Cross-module coupling**: maintain clear boundaries by funnelling interactions
  through asset caches and telemetry interfaces; avoid direct runtime dependencies.

## Open Questions
- Should remeshing run synchronously during asset import or as an asynchronous
  post-process triggered by tooling/runtime heuristics?
- Do we require compatibility with external mesh processing libraries (e.g., OpenMesh,
  libigl) or will we implement bespoke kernels?
- How should we prioritise non-manifold mesh support versus emitting validation
  errors?
- What timeline should we target for GPU acceleration feasibility studies?

Feedback on this RFP should be captured via follow-up issues or ADRs prior to
commencing implementation work under subsequent geometry tasks.
