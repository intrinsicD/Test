# Review — AS-320 Material Persistence Strategy

## Summary
Design note introduces a canonical `.material.json` schema, dependency wiring, and telemetry requirements so materials can be
persisted, reloaded, and streamed consistently across the assets pipeline.

## Architectural Impact
- Aligns with `AI-001` generational handle policy by binding materials only after dependencies resolve.
- Supports `AI-002` async streaming by modelling dependency graph scheduling and telemetry hooks.
- Coordinates with `CC-002` hot reload infrastructure, defining debounce semantics and diagnostics expectations.

## Findings

### Critical Issues 🔴
None.

### Warnings ⚠️
None.

### Suggestions 💡
1. Track sampler asset decision in follow-up
   - Rationale: schema leaves sampler embedding vs. dedicated cache undecided; codifying outcome avoids divergence with rendering module updates.
   - Follow-up task: Convert open question into execution item when scheduling material cache implementation.

2. Link render state override alignment to `RE-530`
   - Rationale: emphasising dependency on backend parity work keeps roadmap cross-references actionable for reviewers.
   - Follow-up task: Update rendering module roadmap alongside implementation to ensure schemas stay in sync.

## Documentation Status
- [x] `docs/design/material_persistence_strategy.md`
- [x] `docs/modules/assets/README.md`
- [x] `docs/modules/assets/ROADMAP.md`
- [x] `docs/ROADMAP.md`
- [x] `README.md`

## Test Coverage
- Documentation only; `python scripts/validate_docs.py` should remain part of landing checklist.

## Follow-Up Work
- [ ] Decide on sampler cache approach before implementing loader.
- [ ] Mirror render state override schema decisions in rendering documentation when coding begins.

## Verdict
- [x] ✅ Approve — design ready for implementation with noted follow-ups.
