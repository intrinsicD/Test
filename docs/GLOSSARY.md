# Glossary

A comprehensive reference of terms, acronyms, and identifiers used throughout the engine documentation and codebase.

---

## Initiative Identifiers

### Cross-Cutting Initiatives (DC, AI, RT, CC, TI)

| ID | Full Name | Description |
|----|-----------|-------------|
| **DC-001** | Design Correction 001 | Subsystem interfaces and plugin discovery in runtime module |
| **DC-003** | Design Correction 003 | SDL backend implementation for platform abstraction |
| **DC-004** | Design Correction 004 | Standardize error handling on `engine::Result<T, Error>` across modules |
| **AI-001** | Architecture Improvement 001 | Handle-based lifetime management and validation hooks |
| **AI-002** | Architecture Improvement 002 | Async asset streaming with telemetry and runtime integration |
| **AI-003** | Architecture Improvement 003 | Frame-graph metadata and queue affinity for backend parity |
| **RT-001** | Runtime Task 001 | Animation runtime skinning pipeline |
| **RT-002** | Runtime Task 002 | Physics persistent manifolds and benchmarking |
| **RT-003** | Runtime Task 003 | Vulkan runtime parity and backend guidance |
| **RT-005** | Runtime Task 005 | Scene hierarchy validation and diagnostics |
| **RT-006** | Runtime Task 006 | IO signature hardening with fuzzing and telemetry |
| **CC-001** | Cross-Cutting 001 | Telemetry instrumentation and diagnostics viewer |
| **CC-002** | Cross-Cutting 002 | Hot reload infrastructure across caches and backends |
| **TI-001** | Testing Infrastructure 001 | Integration test harness for cross-module validation |

### Module-Specific Initiatives

| Prefix | Module | Example IDs |
|--------|--------|-------------|
| **AN-** | Animation | AN-230 (GPU sampling), AN-240 (state machine authoring) |
| **AS-** | Assets | AS-305 (cancellation hardening), AS-320 (material persistence) |
| **CO-** | Compute | CO-150 (cycle detection), CO-160 (CUDA presets), CO-170 (runtime integration) |
| **CR-** | Core | CR-125 (lifecycle audit), CR-130 (config refresh), CR-135 (dependency diagnostics) |
| **GE-** | Geometry | GE-205 (normal recompute), GE-212 (remeshing RFP), GE-220 (telemetry alignment) |
| **IO-** | Input/Output | IO-221 (signature catalogue), IO-230 (error catalog), IO-240 (telemetry) |
| **MA-** | Math | MA-110 (SIMD validation), MA-118 (solver stability), MA-130 (conversion telemetry) |
| **PH-** | Physics | PH-430 (collision throughput telemetry) |
| **PL-** | Platform | PL-215 (SDL backend checklist) |
| **RE-** | Rendering | RE-530 (backend validation tooling) |
| **SC-** | Scene | SC-220 (docs refresh), SC-225 (hierarchy diagnostics), SC-230 (alert policy) |
| **TL-** | Tools | TL-101 (viewer CLI), TL-110 (docs refresh), TL-115 (Chrome trace export) |
| **PY-** | Python | PY-001 (core bindings and .pyi stubs) |

---

## Technical Terms

### Architecture & Systems

| Term | Definition |
|------|------------|
| **ADR** | Architecture Decision Record. Binding documents stored in `docs/specs/` that define architectural choices until superseded. |
| **Backlog** | Module-specific work queue documented in `docs/modules/<name>/BACKLOG.md`. Distinct from the central roadmap which tracks cross-cutting initiatives. |
| **ECS** | Entity-Component-System. The architectural pattern used by the scene module, implemented via EnTT. |
| **Frame Graph** | Dependency graph describing rendering passes, resources, and synchronization. Defined in `docs/specs/ADR_0003_RUNTIME_FRAME_GRAPH.md`. |
| **Generational Handle** | Resource identifier combining an index and generation counter to detect stale references. See `docs/design/RESOURCE_MANAGEMENT.md`. |
| **RFP** | Request for Proposal. Design documents outlining requirements for major features (e.g., `GE_212_REMESHING_PARAMETERIZATION_RFP.md`). |
| **RuntimeHost** | Entry point that orchestrates animation, physics, geometry, and rendering submission. Documented in `docs/modules/runtime/README.md`. |
| **Spatial Index** | Acceleration structure (kd-tree, octree) maintained by geometry module to speed spatial queries. |
| **Task Record** | Markdown file under `docs/backlog/active/` (archived in `docs/backlog/archive/`) containing goal, inputs, constraints, deliverables, and acceptance checklist. |

### Rendering & Graphics

| Term | Definition |
|------|------------|
| **Backend** | Graphics API implementation (Vulkan, DirectX 12, OpenGL) that plugs into the frame-graph scheduler. |
| **Command Encoder** | Abstraction for recording GPU commands into a command buffer within a frame-graph pass. |
| **Frame-Graph Pass** | Single rendering operation (e.g., shadow map generation, geometry pass) with declared resource dependencies. |
| **Queue Affinity** | Assignment of frame-graph passes to specific GPU queues (graphics, compute, transfer) for optimal scheduling. |
| **Resource Descriptor** | Metadata describing GPU resource properties (format, usage, layout) used by the frame-graph compiler. |
| **Submission** | Act of sending recorded command buffers to the GPU for execution. |

### Animation & Deformation

| Term | Definition |
|------|------------|
| **Blend Tree** | Hierarchical structure for combining multiple animation clips with weighted blending. |
| **Clip Sampling** | Evaluation of animation keyframes at a specific time to produce joint poses. |
| **Joint Pose** | Transform (translation, rotation, scale) for a single bone/joint in a skeletal rig. |
| **Linear Blend Skinning (LBS)** | Deformation technique that transforms mesh vertices based on weighted joint influences. |
| **Rig Binding** | Association between mesh vertices and skeletal joints with blend weights. |
| **State Machine** | Animation controller that transitions between clips based on conditions and parameters. |

### Geometry Processing

| Term | Definition |
|------|------------|
| **Halfedge** | Data structure representing mesh connectivity where each edge is split into two directed halfedges. |
| **kd-tree** | K-dimensional binary tree used for spatial partitioning and nearest-neighbor queries. |
| **Octree** | Hierarchical spatial structure subdividing 3D space into eight octants recursively. |
| **Point Cloud** | Set of 3D points, potentially with attributes (normals, colors), without explicit connectivity. |
| **Remeshing** | Process of rebuilding mesh topology to improve quality, uniformity, or resolution. |
| **Surface Mesh** | 3D geometry representation with explicit vertices, edges, and faces. |

### Physics & Simulation

| Term | Definition |
|------|------------|
| **Broad Phase** | Coarse collision detection step that quickly identifies potentially overlapping objects (e.g., sweep-and-prune). |
| **Collider** | Simplified geometric shape (sphere, capsule, AABB, mesh) used for collision detection. |
| **Contact Manifold** | Set of contact points between two colliding objects, cached across frames for stability. |
| **Narrow Phase** | Precise collision detection computing exact contact points between nearby objects. |
| **Rigid Body** | Physics object with mass, velocity, and angular momentum that responds to forces. |
| **Sweep-and-Prune** | Broad-phase algorithm sorting objects along axes to find overlapping bounding volumes. |

### Asset Management

| Term | Definition |
|------|------------|
| **Asset Cache** | Generational handle pool storing loaded resources (meshes, textures, shaders, materials). |
| **Async Streaming** | Background loading of assets without blocking the main thread, with cancellation support. |
| **Hot Reload** | Runtime detection and reloading of modified assets via filesystem watcher callbacks. |
| **Resource Pool** | Dense storage for resources with free-list recycling and generational handle validation. |

### Testing & Quality

| Term | Definition |
|------|------------|
| **CTest** | CMake's testing framework used to run C++ unit and integration tests. |
| **Fuzzing** | Automated testing technique feeding malformed/random inputs to find bugs (e.g., libFuzzer for IO). |
| **Integration Test** | Cross-module test validating interactions between subsystems. |
| **Regression Test** | Test added when fixing a bug to prevent the issue from reoccurring. |
| **Smoke Test** | Basic validation ensuring core functionality works after changes. |

### Telemetry & Diagnostics

| Term | Definition |
|------|------------|
| **Diagnostics Viewer** | CLI/UI tool (`scripts/diagnostics/telemetry_viewer.py`) for inspecting runtime telemetry snapshots. |
| **Metric** | Named, timestamped measurement (counter, gauge, histogram) emitted by instrumented code. |
| **Telemetry Schema** | Standard format for metrics defined in `docs/design/TELEMETRY_SCHEMA.md`. |
| **Instrumentation** | Adding telemetry emission points to code. See `docs/design/TELEMETRY_INSTRUMENTATION_GUIDE.md`. |

---

## Acronyms

| Acronym | Expansion |
|---------|-----------|
| **AABB** | Axis-Aligned Bounding Box |
| **ADR** | Architecture Decision Record |
| **API** | Application Programming Interface |
| **CI** | Continuous Integration |
| **CLI** | Command-Line Interface |
| **CPU** | Central Processing Unit |
| **CUDA** | Compute Unified Device Architecture (NVIDIA GPU programming) |
| **DX12** | DirectX 12 (Microsoft graphics API) |
| **ECS** | Entity-Component-System |
| **FFI** | Foreign Function Interface |
| **GLFW** | Graphics Library Framework (windowing library) |
| **GPU** | Graphics Processing Unit |
| **GUI** | Graphical User Interface |
| **IO** | Input/Output |
| **LBS** | Linear Blend Skinning |
| **MVP** | Minimum Viable Product |
| **OpenGL** | Open Graphics Library |
| **PEP** | Python Enhancement Proposal |
| **PR** | Pull Request |
| **RFP** | Request for Proposal |
| **SDL** | Simple DirectMedia Layer (windowing/input library) |
| **SIMD** | Single Instruction, Multiple Data |
| **UI** | User Interface |
| **VK** | Vulkan (Khronos graphics API) |

---

## Module Names & Responsibilities

| Module | Purpose |
|--------|---------|
| **Animation** | Clip sampling, blend trees, state machines, skeletal deformation |
| **Assets** | Resource caching, hot reload, async streaming, handle management |
| **Compute** | Kernel dispatch, CUDA interop, GPU compute abstraction |
| **Core** | ECS registry (EnTT), subsystem discovery, lifecycle management |
| **Geometry** | Mesh/point-cloud processing, spatial indices, procedural primitives |
| **IO** | Import/export, file format detection, plugin infrastructure |
| **Math** | Vector/matrix/quaternion primitives, transforms, solvers |
| **Physics** | Rigid-body simulation, collision detection, constraint solving |
| **Platform** | Windowing (GLFW/SDL), input handling, filesystem abstraction |
| **Rendering** | Frame-graph execution, backend abstraction (Vulkan/DX12/GL) |
| **Runtime** | Main loop orchestration, subsystem integration, telemetry |
| **Scene** | Entity management, hierarchy, transform propagation, serialization |
| **Tools** | Editor scaffolding, profiler, diagnostics viewer, Dear ImGui integration |

---

## File & Directory Conventions

| Path Pattern | Purpose |
|--------------|---------|
| `docs/specs/ADR-*.md` | Architecture Decision Records (binding) |
| `docs/design/*.md` | Deep-dive design documents and guides |
| `docs/modules/<name>/README.md` | Module overview and API documentation |
| `docs/modules/<name>/BACKLOG.md` | Module-specific work queue |
| `docs/backlog/active/T-*.md` | Individual task records with acceptance criteria |
| `docs/backlog/active/YYYY-MM-DD-sprint-*.md` | Sprint planning documents |
| `docs/archive/` | Historical artifacts (prints, reviews, completed tasks) |
| `engine/<module>/include/` | Public C++ headers |
| `engine/<module>/src/` | C++ implementation files |
| `engine/<module>/tests/` | Module-specific unit tests |

---

**Maintenance:** Add new terms as they become part of regular discussions or when onboarding feedback reveals ambiguity. Keep alphabetized within sections for easy lookup.

**Last updated:** 2025-10-22 (Expanded from 7 to 100+ entries)
