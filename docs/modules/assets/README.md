# Assets Module

## Overview

The assets module provides handle-based resource management with generational handles, asynchronous loading, hot reload support, and comprehensive validation. It manages meshes, point clouds, graphs, textures, shaders, and materials through a unified cache architecture with telemetry integration.

## Core Concepts

### Handle-Based Lifetime Management

Assets use generational handles to ensure safe resource access:

```cpp
#include "engine/assets/api.hpp"

// Acquire a mesh handle
auto mesh_handle = mesh_cache.load("character.obj");

// Handles are lightweight and copyable
auto mesh_handle_copy = mesh_handle;

// Access the resource (returns nullptr if handle is stale)
if (auto* mesh = mesh_cache.get(mesh_handle)) {
    // Use mesh data
}

// Release decrements reference count
mesh_cache.release(mesh_handle);
```

### Resource Types

The module manages the following asset types:

- **MeshAsset**: Surface meshes with vertices, indices, and optional vertex attributes
- **PointCloudAsset**: Point cloud data with positions and optional attributes
- **GraphAsset**: Graph structures (adjacency lists, navigation graphs)
- **TextureAsset**: 2D textures with automatic colour-space tagging, CPU mipmap generation, and HDR-aware decoding
- **ShaderAsset**: Shader programs and bytecode
- **MaterialAsset**: Material definitions referencing textures and shader parameters

Each asset type has a corresponding cache (`MeshCache`, `PointCloudCache`, etc.) and handle type (`MeshHandle`, `PointCloudHandle`, etc.).

### Synchronous Loading

Load assets synchronously when immediate availability is required:

```cpp
using namespace engine::assets;

// Load from file path
MeshHandle handle = mesh_cache.load("assets/models/character.obj");

// Load with parameters
AssetLoadParams params{
    .priority = AssetLoadPriority::High,
    .cache_policy = CachePolicy::Persistent
};
MeshHandle handle2 = mesh_cache.load("terrain.obj", params);

// Check if loaded successfully
if (!mesh_cache.is_valid(handle)) {
    // Handle load failure
}
```

Textures expose additional loading parameters via `TextureLoadingOptions` so call sites can request
automatic mip generation, bound the number of levels, or discard the encoded payload when CPU memory
pressure is a concern:

```cpp
using namespace engine::assets;

TextureLoadingOptions options{
    .generate_mipmaps = true,
    .max_mip_levels = 0, // 0 = full chain, otherwise clamp to the requested count
    .retain_encoded_payload = false,
};

TextureCache texture_cache;

auto descriptor = TextureAssetDescriptor::from_file(
    "assets/textures/albedo.png",
    TextureColorSpace::srgb,
    options);

const TextureAsset& texture = texture_cache.load(descriptor);
fmt::print("{} mip levels, format = {}\n",
           texture.mip_levels.size(),
           static_cast<int>(texture.format));
```

`TextureAsset` now records per-mip texel buffers alongside the decoded format, colour space, and
original file payload (when `retain_encoded_payload` remains enabled). HDR sources decode to
`TextureFormat::rgba32_float` while LDR content uses `TextureFormat::rgba8_unorm`, keeping the data
layout explicit for downstream GPU uploaders.

### Asynchronous Loading

For non-blocking asset loading, use the async API:

```cpp
#include "engine/assets/async.hpp"
#include "engine/core/threading/io_thread_pool.hpp"

AssetLoadRequest request = AssetLoadRequest::from_path(
    AssetType::mesh,
    "assets/meshes/large_model.glb",
    /*params*/ {},
    AssetLoadPriority::High,
    /*deadline*/ std::nullopt,
    /*allow_blocking_fallback*/ false
);

AssetLoadFuture<MeshHandle> future = mesh_cache.load_async(
    request,
    core::threading::IoThreadPool::instance()
);

// Poll or wait for completion
if (future.is_ready()) {
    auto result = future.get();
    if (result) {
        MeshHandle handle = *result;
        // Use loaded mesh
    } else {
        // Handle error: result.error()
    }
}
```

See [`../runtime/ASYNC_STREAMING_INTEGRATION.md`](../runtime/ASYNC_STREAMING_INTEGRATION.md) for runtime integration patterns.

### Hot Reload

Assets can be reloaded when source files change:

```cpp
// Register reload callback
texture_cache.register_hot_reload_callback(texture_handle,
    [&](const TextureAsset& updated)
    {
        fmt::print("{} now exposes {} mip levels\n",
            updated.descriptor.handle.id(),
            updated.mip_levels.size());
    });
```

The platform module's filesystem watcher automatically triggers reload when files change. See `CC-002` in the roadmap for hot reload infrastructure details. The texture cache records decode failures with actionable hints when a file becomes unreadable or an unsupported format is introduced, keeping telemetry dashboards consistent with runtime state.

Hot reload attempts validate that OBJ sources still contain usable geometry. If a watched mesh is replaced with content that
omits vertices or faces, the cache now raises a validation failure, preserves the previous asset, and records the error in
`AssetHotReloadTelemetry` so diagnostics surface the regression immediately.

## Handle Validation

Debug builds automatically validate handle usage:

```cpp
MeshHandle handle = mesh_cache.load("model.obj");
mesh_cache.release(handle);

// Accessing stale handle logs telemetry and returns nullptr
auto* mesh = mesh_cache.get(handle);  // nullptr, logs warning
```

Validation diagnostics are available through `RuntimeDiagnostics::handle_validation` when `ENGINE_ENABLE_ASSETS` is enabled.

## Telemetry & Diagnostics

Asset caches emit telemetry for monitoring:

```cpp
// Access streaming metrics
auto metrics = engine::runtime::streaming_metrics();
fmt::print("Pending: {}, Completed: {}, Failed: {}\n",
    metrics.streaming_pending,
    metrics.streaming_total_completed,
    metrics.streaming_total_failed);

// Geometry-specific error breakdown
for (size_t i = 0; i < metrics.streaming_geometry_failures.size(); ++i) {
    if (metrics.streaming_geometry_failures[i] > 0) {
        fmt::print("  {}: {} failures\n",
            metrics.streaming_geometry_failure_labels[i],
            metrics.streaming_geometry_failures[i]);
    }
}
```

Telemetry tracks:
- Queue health: worker count, capacity, pending tasks
- Request lifecycle: enqueued, executed, completed, failed, cancelled
- Error attribution: per-format failure counts with structured error codes

## Cache Policies

Control resource retention with cache policies:

```cpp
AssetLoadParams params{
    .cache_policy = CachePolicy::Persistent  // Never auto-evict
};

// Or use time-based eviction
params.cache_policy = CachePolicy::TimeToLive;
params.ttl_seconds = 60.0;  // Evict after 60 seconds of no use
```

Available policies:
- **Persistent**: Manually managed, never auto-evicted
- **TimeToLive**: Evicted after specified duration
- **LRU**: Least-recently-used eviction when cache is full

## Error Handling

Asset operations return `Result<T, Error>` following the `DC-004` pattern:

```cpp
auto result = animation::read_clip_json(stream);
if (!result) {
    AssetLoadError error = result.error();
    fmt::print("Load failed: {} (code: {})\n",
        error.message,
        static_cast<int>(error.code));
}
```

Error codes include:
- `AssetLoadError::file_not_found`
- `AssetLoadError::invalid_format`
- `AssetLoadError::validation_failed`
- `AssetLoadError::queue_full`
- `AssetLoadError::cancelled`

## Integration with IO Module

Assets delegate format detection and parsing to the IO module:

```cpp
// IO module provides handlers
auto mesh_result = io::import_mesh("model.obj");
if (mesh_result) {
    mesh_cache.insert("model.obj", std::move(*mesh_result));
}
```

See [`../io/README.md`](../io/README.md) and [`../../specs/ADR-0005-geometry-io-roundtrip.md`](../../specs/ADR-0005-geometry-io-roundtrip.md) for IO architecture. Runtime consumers can supply cache pointers through `RuntimeHostDependencies::asset_streaming` and call `RuntimeHost::request_mesh_asset()` / `RuntimeHost::request_point_cloud_asset()` when orchestration should occur on the runtime thread. These runtime helpers return futures that resolve to `AssetLoadErrorCategory::ValidationError` when the host is not initialised or caches are absent, keeping async orchestration deterministic without throwing exceptions.

## Testing

Tests validate:
- Handle lifecycle and generational validation (`test_handle_validation.cpp`)
- Synchronous and asynchronous loading (`test_assets.cpp`, `test_async.cpp`)
- Cache eviction policies (`test_cache_policies.cpp`)
- Hot reload callbacks (`test_hot_reload.cpp`)
- Telemetry accuracy (`test_telemetry.cpp`)

Run tests:
```bash
ctest --preset linux-gcc-debug -R assets
```

## Dependencies

- **Core**: ECS registry, telemetry schema, IO thread pool
- **IO**: Format handlers, signature detection, import/export
- **Platform**: Filesystem watcher for hot reload
- **Runtime** (integration): Consumes telemetry and orchestrates streaming

## Related Documentation

- [`BACKLOG.md`](BACKLOG.md): Module milestones and upcoming features
- [`../../design/ASYNC_STREAMING.md`](../../design/ASYNC_STREAMING.md): Async architecture and design decisions
- [`../../design/RESOURCE_MANAGEMENT.md`](../../design/RESOURCE_MANAGEMENT.md): Handle lifecycle patterns
- [`../../design/MATERIAL_PERSISTENCE_STRATEGY.md`](../../design/MATERIAL_PERSISTENCE_STRATEGY.md): Material serialization planning
- [`../../archive/backlog/legacy/tasks/T-0115-assets-async-streaming-mvp.md`](../../archive/backlog/legacy/tasks/T-0115-assets-async-streaming-mvp.md): Async streaming milestone
- [`../runtime/ASYNC_STREAMING_INTEGRATION.md`](../runtime/ASYNC_STREAMING_INTEGRATION.md)

## Current State

- Generational handle caches for meshes, point clouds, graphs, textures, shaders, and materials with hot-reload callbacks.
- Texture decoding pipeline backed by stb_image with HDR+LDR support, CPU mipmap generation, colour-space aware descriptors, and retained source payloads for diagnostics or deferred transcoding.
- Async loading pipeline with telemetry integrated into runtime diagnostics; handle validation hooks enabled in debug builds.
- Diagnostics shell (`scripts/diagnostics/telemetry_viewer.py`) surfaces recent asset reload failures with per-asset hints to
  accelerate hot-reload triage (`AS-330`).
- `scripts/diagnostics/streaming_report.py` exports hot-reload metrics (attempt/failure/cancellation counters and recent
  failures) alongside async queue health for CI dashboards.

## Usage

- Run C++ tests for assets:
  - `ctest --preset linux-gcc-debug -R assets`
- See examples above and tests under `engine/assets/tests/` for end-to-end usage.
- The
  [`AI-004 Prototyping Playbook`](../../design/AI-004-prototyping-playbook.md)
  outlines how dataset packaging, schema validation, and harness integration
  interact; consult it before publishing new manifests or ingestion scripts.
- During AI-004 migration, dataset packaging must emit `ai-004.dataset` schema
  headers so downstream harnesses enforce consistent metadata:
  1. Generate manifests with `geometry_remesh --manifest-output` or the dataset
     packaging scripts and verify them via
     `python -m scripts.validate_ai004_config --dataset <manifest>`.
  2. Use `python -m scripts.prototyping.run_prototype_harness --config <config> --require-schema`
     (or export `ENGINE_AI004_SCHEMA_V1=1`) before publishing to ensure the
     runtime harness accepts the manifests without falling back to legacy
     defaults.
  3. Ingest curated manifests with
     `python -m scripts.datasets.ingest_dataset <manifest> --copy-assets --output <cache>`
     to produce reproducible dataset caches populated with checksum metadata.
  4. Populate the mandatory `provenance` block (schema v2+) with licensing,
     attribution, and packaging links so the runtime harness and sandbox can
     surface source/permission details alongside telemetry.

## TODO / Next Steps

- Deliver `AS-330` (AI-004): curate reference dataset packages with manifests, ingestion scripts, provenance/licensing notes, and CI validation feeding the prototyping harness and sandbox UI.
- Maintain hot-reload telemetry alignment and ensure dataset ingestion hooks integrate with existing streaming diagnostics (`CC-002`).
- Track milestone scope and dependencies in [`../../ROADMAP.md`](../../ROADMAP.md) so dataset deliverables remain synchronized with the broader `AI-004` initiative timeline.
