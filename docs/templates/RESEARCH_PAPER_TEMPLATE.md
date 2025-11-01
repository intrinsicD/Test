# Research Paper Integration Template

> Copy this file when documenting a new research algorithm integration.
> Keep all sections even if certain items are “N/A” so readers can audit the
> rationale quickly.

## Algorithm
- **Name:** <!-- e.g. "Fast Signed Distance Fields" -->
- **Module:** <!-- e.g. geometry -->
- **Interface:** <!-- entry points touched (class/function/path) -->
- **Commit / Revision:** <!-- Git SHA or tag that landed the work -->

## Citation
- **Paper:** <!-- Title of the reference paper -->
- **Authors:** <!-- Author list -->
- **Venue:** <!-- Conference / journal -->
- **Year:** <!-- Publication year -->
- **DOI / URL:** <!-- Persistent link -->

## Summary
- **Purpose:** <!-- What problem does the algorithm solve? -->
- **Key ideas:** <!-- Discrete differential geometry, optimisation trick, etc. -->
- **Adaptations:** <!-- Departures from the paper required for the engine -->

## Integration Notes
- **Entry points:** <!-- Which subsystems or pipelines call into the algorithm -->
- **Dependencies:** <!-- Data structures, third-party libs, build flags -->
- **Parameters:** <!-- Tunables exposed to users (with defaults & rationale) -->
- **Telemetry / Diagnostics:** <!-- Metrics exported, logging hooks -->

## Validation
- **Test coverage:** <!-- Unit, integration, benchmarking suites -->
- **Benchmark results:** <!-- Quantitative comparison vs. paper claims -->
- **Visual inspection:** <!-- Screenshots, qualitative assessments -->

## Known Limitations
- **Differences from paper:** <!-- e.g. CPU-only, reduced dimension -->
- **Performance trade-offs:** <!-- Complexity, memory footprint, approximations -->
- **Follow-up work:** <!-- TODOs or future experiment ideas -->

## References & Artefacts
- **Datasets:** <!-- Links to manifests or assets -->
- **Telemetry captures:** <!-- Paths to logs / JSON summaries -->
- **Related tasks:** <!-- Backlog IDs, ADRs, review notes -->
