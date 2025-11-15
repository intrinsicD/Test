---
id: RE-430
title: Implement WASD first-person camera movement controller
status: review
priority: P1
area: rendering
size: S
owner: agent
gates: [tests, docs]
relates_to: [bundle:A]
blocked_on: []
links: []
---

# Task RE-430 — Implement WASD First-Person Camera Movement Controller

## Intent

Deliver a reusable first-person camera controller that reads WASD keyboard input so runtime samples and tools can translate cameras without bespoke input wiring.

---

## Context

**Current State:**
- `engine/rendering/camera_controllers.hpp` exposes orbit and first-person controllers that require callers to provide `CameraControlState` deltas every frame.
- Runtime samples such as `engine/tools/examples/geometry_viewer.cpp` reimplement camera motion by hand because the controllers lack direct keyboard integration.

**Desired State:**
- Rendering module ships a `WASD` controller that consumes `engine::platform::input::InputState` and drives an internal first-person controller.
- Geometry viewer and future runtime tools can instantiate the controller, bind it to the shared input state, and obtain predictable forward/strafe motion without duplicating math.

**References:**
- [`docs/modules/rendering/README.md`](../../docs/modules/rendering/README.md) — camera system overview and invariants.
- [`engine/rendering/camera_controllers.hpp`](../../engine/rendering/include/engine/rendering/camera_controllers.hpp) — existing controller implementations and translation semantics.
- [`engine/platform/input/input_state.hpp`](../../engine/platform/include/engine/platform/input/input_state.hpp) — keyboard state queried by runtime layers.

---

## Design / Plan

### Constraints

- Keep controller logic within the rendering module alongside existing camera controllers; avoid duplicating `InputState` APIs elsewhere.
- Normalise aggregated WASD direction vectors so diagonal movement preserves constant speed.
- Preserve deterministic updates by scaling movement speed using `delta_seconds` identical to other controllers.
- Provide documentation updates in the rendering module README describing the new controller.

### API / Data Sketch

```cpp
class WasdCameraController final : public engine::rendering::FirstPersonCameraController {
 public:
  WasdCameraController(Camera& camera,
                       engine::platform::input::InputState& input_state,
                       engine::math::vec3 position = {0.0F, 0.0F, 0.0F}) noexcept;

  void set_move_speed(float units_per_second) noexcept;
  [[nodiscard]] float move_speed() const noexcept;

  void update(const CameraControlState& state, float delta_seconds) noexcept override;

 private:
  engine::math::vec3 resolve_wasd_direction() const noexcept;
  std::reference_wrapper<engine::platform::input::InputState> input_state_;
  float move_speed_{5.0F};
};
```

### Edge Cases & Failure Modes

- **No keys pressed:** Controller should fall back to the provided `CameraControlState` without injecting translation deltas.
- **Conflicting inputs (W+S / A+D):** Opposing keys must cancel so translation net-zero.
- **Diagonal movement:** Normalise XY plane vectors to avoid √2 speed gains when two keys are pressed.

### Test Plan

- **Unit Tests:**
  - Verify holding `W` advances the camera forward by `move_speed * delta_seconds` units.
  - Verify `W+D` produces diagonally normalised translation (no faster than axial movement).
  - Confirm no key presses preserve the camera position.
- **Integration Tests:**
  - Covered through geometry viewer/manual validation once integrated (future task).
- **Regression Tests:**
  - Extend existing camera controller test suite with the new coverage to ensure future refactors keep WASD behaviour intact.

### Tool Integration

**Profiling:**
- [ ] Not required — controller is lightweight and not on a hot path.

**Diagnostic UI:**
- [ ] No ImGui integration for this task.

**Benchmark Automation:**
- [ ] Not applicable.

**Configuration Management:**
- [ ] Not applicable.

---

## Steps

1. [x] Research/design: Loaded rendering camera controller and input state documentation to confirm integration points.
2. [x] Implement core functionality in `engine/rendering/include/engine/rendering/camera_controllers.hpp`.
   - Added `WasdCameraController` that composes the first-person controller and normalises keyboard direction vectors.
3. [x] Add unit tests in `engine/rendering/tests/test_camera.cpp` covering WASD motion edge cases.
   - Extended the camera test suite with forward, diagonal, and external translation scenarios.
4. [x] Update module README with new capabilities.
   - Documented the new controller in the rendering README camera section.
5. [x] Validate backlog/roadmap alignment once change lands.
   - Registered `RE-430` under Bundle A in `hybrid_workflow/ROADMAP.md`.
6. [x] Run required tests and document evidence.
   - Configured the `linux-gcc-debug-mock` preset to avoid host GLFW dependencies, built `engine_rendering_tests`, and executed the `CameraControllers` suite directly.
   - Documentation validation succeeded via `python scripts/validate_docs.py`.
7. [x] Prepare PR with summary referencing this task file.
   - Drafted PR "Add WASD first-person camera controller" referencing this backlog entry.

---

## Evidence

### Test Results

```bash
cmake --preset linux-gcc-debug-mock
cmake --build --preset linux-gcc-debug-mock --target engine_rendering_tests
out/build/linux-gcc-debug-mock/engine/rendering/tests/engine_rendering_tests --gtest_filter=CameraControllers.*
python scripts/validate_docs.py
```

**Test Summary:**
- Unit tests: Pass — `engine_rendering_tests` (`CameraControllers` suite) built with the mock preset.
- Integration tests: Not run (pending future geometry viewer integration).
- Documentation validation: Pass (`python scripts/validate_docs.py`).

### Performance (if applicable)

Not applicable.

**Artifacts:**
- Pending.

### Quality Gate Sign-offs

| Gate | Status | Owner | Evidence |
|------|--------|-------|----------|
| tests | [x] | QA/Test | `CameraControllers` suite via mock preset (`6b746a†L1-L21`) |
| docs | [x] | Docs/DevRel | `python scripts/validate_docs.py` (`e51e97†L1-L2`) |

### Updated Files

- `engine/rendering/include/engine/rendering/camera_controllers.hpp`
- `engine/rendering/tests/test_camera.cpp`
- `docs/modules/rendering/README.md`
- `hybrid_workflow/ROADMAP.md`
