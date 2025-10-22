# IO Module

## Current State
- Provides geometry and animation import/export wrappers with format detection
  and a plugin-ready registry for mesh/point-cloud/graph handlers.
- Implements scaffolding for cache policies and integrates with asset caches.
- Supplies structured error handling as part of `DC-004` migration across
  geometry and animation importers.
- Ships a data-driven geometry signature database at
  `engine/io/signatures/geometry_signatures.json` with an override via the
  `ENGINE_IO_GEOMETRY_SIGNATURE_PATH` environment variable for local testing.
- Curates a geometry detection fuzz corpus under
  `engine/io/tests/corpus/geometry_detection/` with regression tests ensuring
  each seed maps to the expected importer path.
- Geometry import/export plugins now return `GeometryIoResult` values and map
  filesystem or parsing failures to `GeometryIoErrorCode` identifiers, allowing
  callers to branch on `file_not_found`, `io_failure`, or `invalid_argument`
  outcomes without exceptions.
- Geometry import/export operations emit telemetry counters consumed by the
  runtime diagnostics snapshot, covering attempts, successes, and failure
  breakdowns per error code for mesh, point-cloud, and graph handlers.

## Usage
- Build via `cmake --build --preset <preset> --target engine_io`.
- Include `<engine/io/api.hpp>` for import/export utilities and handler
  registration.
- Run `ctest --preset <preset> --tests-regex engine_io` and fuzz harnesses as
  they come online. Follow the
  [IO Signature Detection & Fuzzing Playbook](detection_fuzzing_playbook.md)
  for setup, corpus management, and triage procedures.

## Telemetry

- `io.geometry.requests` — counter tagged with `operation` (`detect_geometry_file`,
  `read_mesh`, `write_mesh`, etc.) recording total attempts.
- `io.geometry.successes` — counter tagged with `operation` tracking successful
  executions.
- `io.geometry.failures` — counter tagged with `operation` and `error`
  (`file_not_found`, `io_failure`, ...) enumerating failure counts by cause.
- Failures also generate structured warnings via `spdlog`, capturing the file
  path and detected format to aid provenance analysis.
- Metrics surface through `engine::runtime::diagnostics().metrics` and the
  `python scripts/diagnostics/runtime_frame_telemetry.py` viewer; see the
  [Telemetry Instrumentation Guide](../../design/telemetry_instrumentation_guide.md)
  for operational details.

## Signature Database

- The canonical geometry signature catalogue lives at
  `engine/io/signatures/geometry_signatures.json`. Each rule specifies the
  geometry kind, resolved format, matching strategy (`byte_prefix`,
  `line_prefix`, or `contains_all`), and optional format hint propagated during
  detection.
- Override the catalogue path via the
  `ENGINE_IO_GEOMETRY_SIGNATURE_PATH` environment variable to experiment with
  custom entries or trimmed datasets in local fuzzing sessions. The override is
  picked up lazily and cached until the next reset.
- Keep the catalogue in sync with fixtures under
  `engine/io/tests/corpus/geometry_detection/` so regression and fuzzing suites
  cover every recorded signature.

## Error Catalog

### Geometry IO

`GeometryIoResult<T>` carries a `GeometryIoErrorCode` when operations fail. The
table below lists the current error identifiers and common remediation steps.

| Code | Typical Source | Remediation |
| --- | --- | --- |
| `file_not_found` | Detection or import invoked with a path that does not exist. | Verify the filesystem path before calling into the IO module. Surface the missing path to operators so they can correct asset packaging or runtime configuration. |
| `io_failure` | Operating system rejects reads/writes (permissions, transient IO errors). | Retry after confirming access rights, disk availability, or remote share status. Include the failing path in telemetry/logs. |
| `invalid_argument` | Callers request an unsupported conversion (e.g., mismatched geometry kind) or provide malformed parameters. | Validate requested formats and arguments before invoking IO. Fix authoring pipelines or configuration that produced invalid inputs. |
| `unsupported_format` | Format detection or explicit requests resolve to a format without an available importer/exporter. | Register the required plugin or convert assets into a supported format. Consider extending the plugin registry for new formats. |
| `plugin_missing` | Registry has no importer/exporter registered for the resolved format. | Ensure `register_default_geometry_io_plugins` (or equivalent project-specific registration) executes before issuing the request. |

### Handling `GeometryIoResult`

```cpp
using engine::io::GeometryIoResult;

GeometryIoResult<void> read_mesh_into_cache(const std::filesystem::path& path)
{
    auto result = engine::io::read_mesh(path, cache.mesh());
    if (!result)
    {
        const auto& error = result.error();
        telemetry.log_error("geometry.io", error.identifier(), error.message());
        return error; // propagate to caller
    }
    return {};
}
```

Always surface the `identifier()` (e.g., `"file_not_found"`) alongside the
optional error message so diagnostics tooling can bucket failures reliably.

## Fuzz Corpus & Harness

- Build the fuzz target with `cmake --preset <preset> -DENGINE_ENABLE_FUZZING=ON`
  followed by `cmake --build --preset <preset> --target engine_io_geometry_fuzz`.
- Curated seeds live under
  `engine/io/tests/corpus/geometry_detection/`; regression tests validate that
  detection maps each file to the correct geometry kind and format.
- Run libFuzzer locally by pointing it at the checked-in corpus:
  `./out/engine_io_geometry_fuzz engine/io/tests/corpus/geometry_detection`.
- When fuzzing infrastructure is unavailable, execute the harness in
  single-shot mode by passing a sample path (`./out/engine_io_geometry_fuzz
  sample.ply`).

Maintain provenance notes for each seed in the corpus README to keep legal
review straightforward and to accelerate future triage.

### Animation Clips

`AnimationIoResult<T>` reports failures using `AnimationIoErrorCode` when
detecting, loading, or saving animation clips.

| Code | Typical Source | Remediation |
| --- | --- | --- |
| `file_not_found` | Detection invoked on a clip path that does not exist. | Verify authoring/export steps and ensure the runtime configuration references deployed assets. |
| `io_failure` | Filesystem rejects read/write operations (permissions, sharing violations). | Retry after confirming permissions and storage availability; surface the failing path via telemetry. |
| `unsupported_format` | File extension/signature does not map to a supported clip encoder. | Convert the clip to JSON or extend the importer with a plugin that handles the required format. |
| `decode_failure` | Clip contents are malformed JSON or fail animation validation. | Inspect the error message for validation hints, correct authoring bugs, and re-export the clip. |
| `serialization_failure` | Saving failed because the clip is invalid or the filesystem reported an error mid-write. | Resolve validation issues or filesystem capacity/permissions before retrying the export. |

```cpp
using engine::io::AnimationIoResult;

AnimationIoResult<engine::animation::AnimationClip>
load_animation_clip(const std::filesystem::path& path)
{
    auto clip = engine::io::animation::load_clip(path);
    if (!clip)
    {
        telemetry.log_error("animation.io", clip.error().identifier(), clip.error().message());
        return clip.error();
    }
    return clip;
}
```

## TODO / Next Steps

- Keep `IO-240` telemetry tasks in sync with the [central roadmap](../../ROADMAP.md) and
  update the execution checklist below when status changes — advances `RT-006`
  and `DC-004`. `IO-230` is now covered by the error catalog above.
- Refer to the [detection & fuzzing playbook](detection_fuzzing_playbook.md) when onboarding
  new formats or refreshing the fuzzing harness.
- Keep the fuzz corpus curated and coordinate CI enablement for the libFuzzer
  harness once shared runners become available.

This module tracks actionable work through the execution checklist below.

## Execution Checklist

| Task ID | Scope | Exit Criteria | Status |
| --- | --- | --- | --- |
| `IO-221` | Integrate signature database + fuzz harness (`RT-006`). | Signature set committed with curated corpus, regression tests, and updated docs; CI enablement tracked separately. | ✅ Done |
| `IO-230` | Publish structured error catalog. | Document error codes and remediation steps in README + design note. | ✅ Done |
| `IO-240` | Align telemetry for import/export failures. | Emit metrics consumed by diagnostics viewer and log failure provenance. | ✅ Done |

See [ROADMAP.md](ROADMAP.md) for timeline and dependencies.
