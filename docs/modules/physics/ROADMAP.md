# Physics Module Roadmap

## Near Term
- Implement contact manifold generation and persistent contact caching to accompany `detect_collisions`, ensuring collisions feed usable constraints.
- Add regression tests for integration and damping behaviour, covering extreme mass ratios and substep configurations.

## Mid Term
- Introduce constraint solvers (e.g., sequential impulses) with extensible constraint descriptors for joints and limits.
- Optimise broadphase by introducing sweep-and-prune structures and spatial partitioning to accelerate collider pairing.

## Long Term
- Support articulated bodies, sleeping/activation heuristics, and authoring tools that visualise colliders and forces during runtime debugging.
