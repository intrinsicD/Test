# Backlog Item AS-330 — Reference Dataset Packages

- **Status**: Planned
- **Priority**: 3
- **Owner**: Assets Lead
- **Module(s)**: Assets, Geometry, Rendering, Python Tooling
- **Goal**: Provide curated, licensed datasets with manifests and ingestion tooling that plug into the AI-004 workflow.

## Summary
AI-004 requires canonical datasets so harness runs are reproducible and comparable. AS-330 selects permissively licensed assets, packages them with manifests and ingestion scripts, and wires them into the runtime harness and sandbox UI. Each package documents provenance, recommended scenarios, and validation checks to guarantee consistent scale and materials.

## Definition of Done
- [ ] Publish dataset manifests covering at least geometry, rendering, and animation categories with provenance and licensing notes.
- [ ] Automated ingestion script downloads, validates (checksums), and registers datasets with engine asset caches.
- [ ] Runtime harness and sandbox UI list available datasets via the shared configuration schema.
- [ ] Assets module README and prototyping playbook document installation, troubleshooting, and reproducibility checklist.

## Dependencies
- [`docs/backlog/active/DC-040-ai-004-configuration-schema.md`](DC-040-ai-004-configuration-schema.md)
- [`docs/backlog/active/RT-320-runtime-prototyping-harness.md`](RT-320-runtime-prototyping-harness.md)

## Related Artefacts
- [`docs/archive/backlog/legacy/tasks/AS-330-reference-dataset-packages.md`](../../archive/backlog/legacy/tasks/AS-330-reference-dataset-packages.md)
- [`scripts/diagnostics/streaming_report.py`](../../../scripts/diagnostics/streaming_report.py) for ingest validation

## Notes
Ensure licensing review completes before publishing manifests. Provide smaller subsets for CI usage to keep runtimes deterministic.
