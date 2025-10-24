
## Example: Adding SAH to Octree

ROLE: Tech Lead
REPO: https://example.com/engine


SCOPE: engine::geometry::Octree
CONTEXT: Issue #231, ADR-014
OBJECTIVE: Implement SAH split heuristic with pluggable cost model.
CONSTRAINTS: C++20, EnTT, SoA nodes, GPU-friendly AABB pool
DELIVERABLES: PR with headers/impl/tests/bench/docs


### Plan

- **Interfaces:** Define a `SplitHeuristic` concept; default implementation uses SAH.
- **Tests:** Cover known partitions, random clouds, and adversarial shapes.
- **Benchmarks:** Measure build time, query time, and memory usage vs. baseline.
- **Documentation:** Provide a Markdown page with diagrams and tunables.

### Handoffs

- **To:** Reviewer and Performance Engineer.
- **After merge:** Librarian adds a pattern write-up for future reference.
