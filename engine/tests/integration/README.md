# Integration Test Harness

The `engine_integration_tests` target exercises cross-module flows that are
critical to the runtime → rendering vertical slice tracked under
[`TI-001`](../../docs/ROADMAP.md#ti-001-integration-suites) and
[`T-0114`](../../docs/tasks/T-0114-testing-integration-suites.md). The harness
runs headless using the mock platform backend and focuses on deterministic
validation across animation, physics, geometry/IO, assets, and rendering.

## Scenarios

1. **Animation ↔ Physics ↔ Runtime:** Drives `RuntimeHost` for several ticks and
   asserts the compute dispatcher executes the animation/physics/geometry
   kernels in order while body positions and joint poses advance deterministically.
2. **Geometry/IO Round-Trip via Assets:** Serialises a translated unit quad to
   OBJ, reloads it through `engine::assets::MeshCache`, and feeds the result into
   `RuntimeHost` to ensure round-tripped vertex data and bounds stay consistent.
3. **Runtime → Rendering Submission:** Configures a renderable entity, records
   resource requirements, and verifies the Vulkan scheduler submission emitted by
   `RuntimeHost::submit_render_graph` matches the expected pass, queue, and draw
   payload.

## Determinism & Environment

- Tests assume `ENGINE_WINDOW_BACKEND=MOCK` so no real windowing surface is
  created. The mock backend is configured automatically by the CMake presets used
  in CI (`linux-gcc-*`).
- Every scenario seeds its own inputs (mesh translation, rig bindings, asset
  identifiers) and avoids random sources to keep telemetry deterministic.
- Temporary files are created with a monotonic counter under
  `std::filesystem::temp_directory_path()` and removed at teardown to prevent
  interference between runs.

## Running the Suite

```bash
cmake --preset linux-gcc-debug
cmake --build --preset linux-gcc-debug --target engine_integration_tests
ctest --preset linux-gcc-debug --tests-regex engine_integration_tests
```

The target links against the full runtime stack, so building it implicitly keeps
animation, physics, geometry, IO, assets, rendering, and scene modules in sync.
When adding scenarios, prefer placing helpers in
`test_runtime_integration.cpp` so they remain co-located with the tests and easy
for reviewers to audit.
