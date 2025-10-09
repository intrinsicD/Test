# Rendering Module Include Path Audit

## Summary

- Relocated the synchronisation primitives to `engine/rendering/include/engine/rendering/resources` so the public API no longer relies on relative includes.
- Re-confirmed that `engine_apply_module_defaults` exposes only the `include/` subtree to dependants, keeping backend- and test-only headers private.
- No other headers currently consumed by public interfaces reside outside the exported include tree; backend schedulers and test helpers remain implementation details.

## Details

- `engine_apply_module_defaults` adds the values passed in `PUBLIC_INCLUDE_DIRS` both to the concrete module target and to the aggregate interface target `engine::headers`. The rendering module only contributes `${CMAKE_CURRENT_SOURCE_DIR}/include` in this list, so the exported include path is restricted to `engine/rendering/include`.\
  _Reference: [`CMakeLists.txt`](../../CMakeLists.txt) and [`engine/rendering/CMakeLists.txt`](../../engine/rendering/CMakeLists.txt)_
- `PRIVATE_INCLUDE_DIRS` affects only the compilation of the module itself. Supplying `${CMAKE_SOURCE_DIR}` in the rendering module makes the implementation files see the whole source tree, but this path is not propagated to consumers.\
  _Reference: [`CMakeLists.txt`](../../CMakeLists.txt)_
- `engine/rendering/include/engine/rendering/gpu_scheduler.hpp` and `frame_graph.hpp` now include `engine/rendering/resources/synchronization.hpp` directly because the header sits under the exported include tree. This ensures downstream projects receive the same include resolution as the module itself.\
  _Reference: [`engine/rendering/include/engine/rendering/gpu_scheduler.hpp`](../../engine/rendering/include/engine/rendering/gpu_scheduler.hpp), [`engine/rendering/include/engine/rendering/frame_graph.hpp`](../../engine/rendering/include/engine/rendering/frame_graph.hpp), and [`engine/rendering/include/engine/rendering/resources/synchronization.hpp`](../../engine/rendering/include/engine/rendering/resources/synchronization.hpp)_
- The remaining headers that live outside `engine/rendering/include` (`engine/rendering/backend/*.hpp` and `engine/rendering/tests/scheduler_test_utils.hpp`) are confined to backend scaffolding and test utilities. They are only consumed from within the rendering module, so keeping them private maintains a clear separation between the exported API and implementation details.\
  _Reference: [`engine/rendering/backend`](../../engine/rendering/backend) and [`engine/rendering/tests`](../../engine/rendering/tests)_

## Recommendations

- Continue publishing headers meant for consumers from `engine/rendering/include` and keep backend/test helpers private. If future resource abstractions need to be shared, place them under `engine/rendering/include/engine/rendering/resources` alongside the synchronisation primitives to avoid reintroducing relative includes.
