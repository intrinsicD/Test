# Result Adoption Opportunities

The `DC-004` migration standardises recoverable error handling around `engine::Result<T, Error>` and reserves exceptions for programmer mistakes or unrecoverable failures.【F:docs/design/ERROR_HANDLING_MIGRATION.md†L5-L36】 Several subsystems still throw for user-supplied input or external resources, creating call sites that cannot inspect structured error metadata. The following candidates should migrate to module-specific `Result` types so tooling and telemetry can treat the errors as recoverable.

## Runtime prototype harness sample
- `parse_double`, `parse_size`, and `parse_command_line` throw `std::invalid_argument`/`std::out_of_range` for malformed CLI values even though callers can surface validation issues to users. Threading a `Result<CommandLineOptions, HarnessError>` (or similar) through the harness entry point would align the native sample with the Python harness’ structured error model.【F:engine/runtime/samples/prototype_harness.cpp†L90-L247】
- `resolve_dataset` throws `HarnessError` when the configuration references an unknown dataset slug. Returning a `Result<const config::DatasetEntry*, HarnessError>` (or a dedicated error enum) lets automation collect slug mismatches without relying on exceptions.【F:engine/runtime/samples/prototype_harness.cpp†L250-L270】

## Compute runtime dispatch demo
- The CLI parser emits numerous `std::invalid_argument` throws for missing or inconsistent arguments (queue counts, timestep positivity, backend availability). These represent recoverable input problems and should propagate via a `Result<CommandLineOptions, DispatchDemoError>` so batch runners can aggregate validation failures instead of catching exceptions.【F:engine/compute/samples/runtime_dispatch_demo/runtime_dispatch_demo.cpp†L287-L527】

## Filesystem watcher
- `normalise_path` and `FilesystemWatcher::watch_file` throw `std::invalid_argument` when the watch path is empty or the callback is missing. A `Result<WatchHandle, FilesystemWatcherError>` would allow clients to distinguish configuration mistakes from polling state transitions while keeping invariants assert-based internally.【F:engine/platform/src/filesystem/watcher.cpp†L17-L84】

## Scene serialization
- `Scene::load` throws `std::runtime_error` for malformed or out-of-sync scene archives (unexpected tokens, missing component data). Converting these checks to return a `Result<void, SceneSerializationError>` would surface precise failure reasons to tooling while letting the caller decide whether to abort, retry, or continue with partial data.【F:engine/scene/src/serialization/serializer.cpp†L107-L215】

## Surface mesh conversion helpers
- The surface-mesh↔halfedge conversion utilities reject degenerate or inconsistent geometry via `std::runtime_error`. Because these functions process external datasets, returning a `Result<void, SurfaceMeshConversionError>` (with codes for non-triangular faces, index mismatches, or non-manifold insertions) would deliver actionable diagnostics to import pipelines without triggering exception unwinding.【F:engine/geometry/src/mesh/surface_mesh_conversion.cpp†L33-L124】
