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

_This draft exists to unblock coordination and will be fleshed out as part of `DC-040`._
