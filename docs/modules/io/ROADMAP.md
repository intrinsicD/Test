# IO Module Roadmap

## Near Term
- Expand format detection heuristics with signature-based inspection to reduce reliance on filename extensions, and add regression tests per codec.
- ✅ Surface structured error reporting from import/export functions (error enums, context strings) so assets and tooling can diagnose failures via `GeometryIoResult` and `GeometryIoErrorCode`.

## Mid Term
- Add asynchronous streaming hooks that load geometry on worker threads and integrate with the asset cache hot-reload pipeline.
- Support incremental import/export for large datasets (chunked point clouds, mesh tiling) to bound memory usage.

## Long Term
- Provide authoring-time validation utilities that scan asset directories, repair metadata, and generate statistics for pipeline dashboards.
