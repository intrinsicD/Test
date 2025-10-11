# Physics Module Roadmap

## Current Observations

- `engine/physics/api.hpp` exposes a lightweight rigid-body world that stores mass properties, integrates linear motion, and optional colliders limited to spheres and axis-aligned bounding boxes. Runtime mass safety clamps to non-negative values before computing the inverse mass to avoid division by zero and now treats non-positive masses as static placeholders that ignore forces and gravity.
- `engine/physics/src/api.cpp` implements Euler integration, naïve $O(n^2)$ pairwise collision checks, and delegates shape intersection queries to `engine::geometry`. Colliders are stored directly on bodies, and collision detection does not persist contact state beyond the current frame.
- Unit tests in `engine/physics/tests/test_module.cpp` validate force application, integration correctness, collider attachment, and a subset of sphere–sphere / sphere–AABB collision scenarios. Coverage does not yet address degenerate masses, sleeping, or resolution strategies.

## Roadmap

### 1. Stabilise the Core World Representation (Short Term)

- **Mass and force handling** – With static body behaviour enforced and degenerate-mass tests in place, focus next on documenting invariants and surfacing samples that exercise static bodies alongside dynamic actors.
- **API factoring** – Move collision-specific helpers out of `api.cpp` into dedicated headers/translation units (as hinted by `engine/physics/src/README.md`) to reduce rebuild costs and clarify ownership boundaries between dynamics and collision subsystems.
- **Instrumentation** – Introduce lightweight profiling hooks and scenario-driven samples to validate stability under typical workloads before expanding functionality.

### 2. Introduce Robust Collision Management (Mid Term)

- **Broad-phase acceleration** – Replace the current $O(n^2)$ sweep with a spatial partition (sweep-and-prune, BVH, or spatial hash) suited to dynamic scenes, and add regression tests to ensure determinism.
- **Contact manifolds** – Persist overlapping pairs and compute contact normals/penetration depths so constraint solvers can operate on consistent manifolds frame-to-frame.
- **Constraint solver** – Implement impulse-based or sequential impulse solvers to resolve collisions, starting with perfectly elastic contacts and expanding towards frictional constraints.

### 3. Advance Dynamics Fidelity (Long Term)

- **Integration schemes** – Provide semi-implicit (symplectic Euler) and optionally higher-order integrators to improve energy behaviour. Tie integrator selection to tests that monitor long-term drift.
- **Sleeping and activation** – Add heuristics to put resting bodies to sleep and wake them when external forces or contacts occur to keep broad-phase queries tractable.
- **Extensible collider set** – Add OBB, capsule, and mesh colliders layered on top of existing geometry predicates, ensuring each new shape ships with dedicated narrow-phase tests.

## Dependencies and Coordination

- Coordinate collision primitive expansion with `engine::geometry` so new predicates land alongside the physics features that consume them.
- Update module documentation (`engine/physics/README.md`, `engine/physics/include/engine/physics/README.md`, and related READMEs) as the roadmap items graduate from planning to implementation.
- Track aggregated progress in the workspace root `README.md` backlog to keep the physics roadmap aligned with adjacent subsystems (scene, runtime, and rendering) and the shared priorities recorded in [docs/global_roadmap.md](../global_roadmap.md).
