# Task Card: AS-330

## Title
Reference Dataset Packages

## Type
- [x] Feature
- [ ] Bug Fix
- [x] Refactor
- [x] Documentation
- [x] Research
- [ ] Performance Optimization

## Priority
- [x] Critical (P0)
- [ ] High (P1)
- [ ] Medium (P2)
- [ ] Low (P3)

## Estimated Effort
2 weeks (assets + tooling)

---

## Description

### Problem Statement
Research teams require canonical datasets (meshes, point clouds, animation clips, materials) with metadata and ingestion scripts to reproduce experiments. Current asset caches rely on ad-hoc imports without provenance tracking, licensing documentation, or integration with the runtime harness, making comparisons to related work unreliable.

### Proposed Solution
Curate a set of permissively licensed datasets covering canonical research cases (geometry processing, rendering, simulation). Package assets with manifests, LFS pointers, and preprocessing scripts. Integrate dataset selection with the prototyping harness (`RT-320`) and sandbox UI (`TL-210`). Provide validation scripts and metadata to ensure consistent scale, units, and material assignments.

### Success Criteria
- Dataset packages stored in versioned manifests with provenance, licensing, and checksum verification.
- Ingestion pipeline automatically registers assets with runtime/engine caches.
- Researchers can select datasets from sandbox UI and harness CLI without manual configuration.
- Documentation describes dataset characteristics, recommended benchmarks, and licensing obligations.

---

## Technical Details

### Scope
**Modules Affected:**
- `engine::assets`
- `engine::geometry`
- `engine::rendering`
- `python::engine3g`
- `scripts::diagnostics`

**Files to Modify:**
- `engine/assets/include/...` dataset registry APIs
- `engine/assets/src/...`
- `python/engine3g/loader.py`
- `scripts/diagnostics/` ingestion scripts
- `docs/modules/assets/README.md`
- `docs/ROADMAP.md`

**New Files:**
- `assets/datasets/<name>/manifest.yaml`
- `scripts/datasets/ingest_dataset.py`
- `docs/design/AS-330-dataset-catalog.md`

### Dependencies
**Depends On:**
- `AI-002` async streaming infrastructure
- `CC-002` hot reload instrumentation

**Blocks:**
- `RT-320` harness dataset loading
- `TL-210` sandbox dataset enumeration
- `CC-310` benchmark scenario definitions

### Related Work
- `docs/design/RESOURCE_MANAGEMENT.md`
- `scripts/diagnostics/streaming_report.py`
- `T-0115-assets-async-streaming-mvp.md`

---

## Acceptance Criteria

### Functional Requirements
- [ ] Curated dataset list with at least three categories (geometry processing, rendering, animation).
    - [x] Automated ingestion script downloads, validates, and registers datasets.
- [ ] Dataset manifest exposes scale, unit, tags, and recommended benchmark scenarios.
- [ ] Runtime harness consumes manifest to prefetch assets and map to materials/shaders.

### Non-Functional Requirements
- [ ] Download/install script completes within 10 minutes on 1 Gbps connection.
- [ ] Storage footprint ≤ 15 GB per dataset bundle.
- [ ] Integrity checks (hash verification) required before ingest completes.

### Testing Requirements
- [ ] Unit tests for manifest parsing and validation.
- [ ] Integration tests cover ingestion workflow and runtime consumption.
- [ ] Smoke test ensures dataset selection works in sandbox UI.
- [ ] Coverage ≥ 85% on new asset ingestion code.

### Documentation Requirements
- [ ] Dataset catalog document with licensing, provenance, and recommended use cases.
- [ ] Update assets README with ingestion workflow and troubleshooting.
- [ ] Add quickstart instructions to prototyping playbook.
- [ ] Provide reproducibility checklist for dataset modifications.

---

## Test Plan

### Unit Tests
```cpp
TEST(DatasetManifest, ParsesMetadata) {
    DatasetManifest manifest = LoadDatasetManifest(test_manifest_path);
    EXPECT_EQ(manifest.scale, 0.01f);
    EXPECT_TRUE(manifest.HasTag("geometry"));
}
```

### Integration Tests
- Execute ingestion script end-to-end, populate dataset cache, and launch runtime harness to verify asset availability.
- Validate failure cases (missing files, invalid checksum) produce actionable errors.

### Performance Tests
- Measure ingestion throughput and ensure CPU/memory usage stays within documented limits.

---

## Implementation Notes

### Design Considerations
- Use YAML manifests validated against JSON schema for deterministic structure.
- Store dataset downloads via git-lfs or external CDN with caching to avoid repo bloat.
- Provide version pinning to support reproducible research citations.
- Enforce schema headers during packaging by running
  `python -m scripts.validate_ai004_config --dataset <manifest>` and
  exercising the prototyping harness with `--require-schema` so assets remain
  compatible once strict validation becomes the default.

### Risks & Mitigations
| Risk | Likelihood | Impact | Mitigation |
|------|------------|--------|------------|
| Licensing conflicts | Low | High | Vet assets with legal/licensing review; prefer CC-BY/CC0 datasets.
| Large download sizes | Medium | Medium | Provide optional subset downloads and compression.
| Manifest drift | Medium | Medium | Automate validation in CI and lock versions in manifest.

### Alternative Approaches
1. **Use external dataset repositories without packaging** → rejected; lacks integration and provenance tracking.
2. **Embed datasets directly in repo** → rejected; bloats repository and complicates licensing.

---

## Deliverables

- [x] Dataset manifests + ingestion scripts
- [ ] Asset registry extensions + tests
- [ ] Documentation + licensing notes
- [ ] CI validation for manifests
- [ ] Linked PRs referencing `AS-330`

---

## Definition of Done

- [ ] Ingestion scripts validated in CI
- [ ] Runtime harness lists datasets successfully
- [ ] Documentation reviewed by assets + research leads
- [ ] Licensing/provenance sign-off recorded
- [ ] Dataset version manifest stored in repo

---

## Assigned To
**Role**: Assets Engineer
**Name**: @assets-lead

## Estimated Timeline
**Start Date**: 2025-11-28
**Target Completion**: 2025-12-12
**Actual Completion**: _TBD_

---

## Notes
- Coordinate with dataset providers for redistribution approval when necessary.
- Provide automated cleanup script to reclaim disk space.

## Progress Log
- 2025-12-04: Licensing packet (assets + legal) submitted for primary geometry datasets; awaiting legal approval by 2025-12-08 to unblock harness manifests.
- 2025-12-16: `geometry_remesh` adds `--manifest-output` to write AI-004 dataset manifest YAML directly, reducing manual steps for dataset ingestion scripts.
- 2025-12-19: Added `assets/datasets/remesh_sample` seed manifest and `scripts/datasets/ingest_dataset.py`
  with regression coverage so packaging workflows emit reproducible cache metadata and validated
  file inventories for AI-004 harness integration.
