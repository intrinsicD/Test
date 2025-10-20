# IO Module

## Current State
- Provides geometry and animation import/export wrappers with format detection
  and a plugin-ready registry for mesh/point-cloud/graph handlers.
- Implements scaffolding for cache policies and integrates with asset caches.
- Supplies structured error handling as part of `DC-004` migration.

## Usage
- Build via `cmake --build --preset <preset> --target engine_io`.
- Include `<engine/io/api.hpp>` for import/export utilities and handler
  registration.
- Run `ctest --preset <preset> --tests-regex engine_io` and fuzz harnesses as
  they come online.

## Error Catalog

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

## TODO / Next Steps

- Track `IO-221` and `IO-240` in the [central roadmap](../../ROADMAP.md) and update the execution checklist below when status changes — advances `RT-006` and `DC-004`. `IO-230` is now covered by the error catalog above.

This module tracks actionable work through the execution checklist below.

## Execution Checklist

| Task ID | Scope | Exit Criteria | Status |
| --- | --- | --- | --- |
| `IO-221` | Integrate signature database + fuzz harness (`RT-006`). | Signature set committed, fuzz target wired into CI, README updated. | 🔄 In Progress |
| `IO-230` | Publish structured error catalog. | Document error codes and remediation steps in README + design note. | ✅ Done |
| `IO-240` | Align telemetry for import/export failures. | Emit metrics consumed by diagnostics viewer and log failure provenance. | 🟢 Todo |

See [ROADMAP.md](ROADMAP.md) for timeline and dependencies.
