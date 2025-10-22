# IO Module

## Overview

The IO module provides file format detection, import/export handlers for meshes and animations, plugin-ready architecture, and structured error reporting. It implements the `DC-004` error handling pattern with `Result<T, Error>` types and includes fuzzing infrastructure for robustness (`RT-006`).

## Format Support

### Mesh Formats

Supported import/export formats:

- **OBJ**: Wavefront OBJ with material libraries (`.obj`, `.mtl`)
- **STL**: Binary and ASCII STL (`.stl`)
- **PLY**: Polygon File Format (`.ply`)
- **OFF**: Object File Format (`.off`)

```cpp
#include "engine/io/api.hpp"

// Import mesh
auto result = io::import_mesh("model.obj");
if (result) {
    geometry::SurfaceMesh mesh = std::move(*result);
} else {
    fmt::print("Import failed: {}\n", result.error().message);
}

// Export mesh
auto export_result = io::export_mesh(mesh, "output.stl", io::MeshFormat::STL);
if (!export_result) {
    fmt::print("Export failed: {}\n", export_result.error().message);
}
```

### Animation Formats

```cpp
// Import animation clip
auto clip_result = io::import_animation("walk.anim");
if (clip_result) {
    animation::AnimationClip clip = std::move(*clip_result);
}

// Export animation clip
auto export_result = io::export_animation(clip, "walk.json", io::AnimationFormat::JSON);
```

## Automatic Format Detection

The IO module detects formats via signatures:

```cpp
// Detect format from file content
auto format = io::detect_mesh_format("unknown_file.dat");

if (format == io::MeshFormat::OBJ) {
    // Handle as OBJ
} else if (format == io::MeshFormat::STL) {
    // Handle as STL
}
```

Format detection uses:
- Magic bytes (binary formats)
- Content heuristics (ASCII formats)
- File extension as fallback

See [`detection_fuzzing_playbook.md`](detection_fuzzing_playbook.md) for signature database details.

## Error Handling

All IO operations return `Result<T, GeometryIoError>`:

```cpp
auto result = io::import_mesh("model.obj");
if (!result) {
    auto error = result.error();
    
    switch (error.code) {
        case io::GeometryIoErrorCode::file_not_found:
            fmt::print("File not found: {}\n", error.path);
            break;
        case io::GeometryIoErrorCode::invalid_format:
            fmt::print("Invalid format at line {}\n", error.line);
            break;
        case io::GeometryIoErrorCode::unsupported_feature:
            fmt::print("Unsupported: {}\n", error.message);
            break;
        // ... handle other codes
    }
}
```

Error codes include:
- `file_not_found`: Path doesn't exist
- `invalid_format`: Parse error
- `unsupported_feature`: Feature not implemented
- `validation_failed`: Imported data fails validation
- `io_error`: Filesystem/stream error
- `out_of_memory`: Allocation failure

## Plugin Architecture

Register custom format handlers:

```cpp
#include "engine/io/plugin/handler_registry.hpp"

class FbxHandler : public io::IMeshFormatHandler {
public:
    std::string_view format_name() const override { return "FBX"; }
    
    std::vector<std::string> extensions() const override {
        return {".fbx"};
    }
    
    io::GeometryIoResult<geometry::SurfaceMesh> import(
        std::istream& stream) override {
        // Parse FBX format
        return mesh;
    }
    
    io::GeometryIoResult<void> export_mesh(
        const geometry::SurfaceMesh& mesh,
        std::ostream& stream) override {
        // Write FBX format
        return {};
    }
};

// Register handler
io::register_mesh_handler(std::make_unique<FbxHandler>());
```

## Validation

Imported data is validated before returning:

```cpp
auto result = io::import_mesh("model.obj");
// Validation happens automatically:
// - Index bounds checking
// - Attribute count consistency
// - Non-degenerate triangles
// - Orientation consistency (optional)

if (!result) {
    // Invalid mesh rejected with detailed error
}
```

Configure validation strictness:

```cpp
io::ImportOptions options{
    .validate_indices = true,
    .validate_normals = true,
    .require_uvs = false,
    .triangulate = true  // Convert polygons to triangles
};

auto result = io::import_mesh("model.obj", options);
```

## Telemetry

IO operations emit structured telemetry:

```cpp
auto telemetry = io::geometry_io_telemetry();

for (const auto& [format_name, metrics] : telemetry.formats) {
    fmt::print("Format: {}\n", format_name);
    fmt::print("  Imports: {} ({} failures)\n",
        metrics.import_count, metrics.import_failures);
    fmt::print("  Exports: {} ({} failures)\n",
        metrics.export_count, metrics.export_failures);
    fmt::print("  Avg duration: {:.3f}ms\n", metrics.avg_duration_ms);
}

// Error code breakdown
for (size_t i = 0; i < io::geometry_io_error_count(); ++i) {
    auto code = static_cast<io::GeometryIoErrorCode>(i);
    auto count = telemetry.error_counts[i];
    if (count > 0) {
        fmt::print("  {}: {} occurrences\n",
            io::error_code_name(code), count);
    }
}
```

Telemetry is integrated with runtime diagnostics:

```cpp
const auto& runtime_diag = runtime::diagnostics();
const auto& io_snapshot = runtime_diag.geometry_io;
// Same data exposed through unified diagnostics
```

## Fuzzing (`RT-006`)

The module includes fuzzing infrastructure for robustness testing:

```cpp
// Fuzz corpus in engine/io/signatures/
// Run with libFuzzer:
// ./io_fuzz_import corpus/ -max_len=1048576
```

Fuzzing targets:
- Format detection (signature validation)
- Parser robustness (malformed input)
- Validation edge cases
- Memory safety

See [`detection_fuzzing_playbook.md`](detection_fuzzing_playbook.md) for fuzzing workflow and corpus management.

## Cache Integration

IO works seamlessly with asset caches:

```cpp
// Assets module delegates to IO
auto mesh_result = io::import_mesh("character.obj");
if (mesh_result) {
    mesh_cache.insert("character.obj", std::move(*mesh_result));
}
```

## Performance

Typical operation times (from `T-0112`):
- OBJ import (10k vertices): ~5ms
- STL binary import (50k triangles): ~3ms
- Format detection: <0.1ms
- Validation: ~0.5ms per 10k vertices

Optimizations:
- Memory-mapped files for large assets
- Parallel parsing for independent sections
- Zero-copy buffer management where possible

## Testing

IO tests validate:
- Format detection accuracy (`test_format_detection.cpp`)
- Import/export roundtrip fidelity (`test_roundtrip.cpp`)
- Error handling and recovery (`test_error_handling.cpp`)
- Plugin registration (`test_plugin.cpp`)
- Telemetry accuracy (`test_telemetry.cpp`)
- Fuzzing (corpus-based regression tests)

Run tests:
```bash
ctest --preset clang-debug -R io
```

Run fuzz tests:
```bash
# Requires libFuzzer build
cmake --preset clang-debug-fuzz
cmake --build --preset clang-debug-fuzz
./build/engine/io/io_fuzz_import corpus/ -runs=10000
```

## Dependencies

- **Geometry**: Target data structures (SurfaceMesh, PointCloud)
- **Animation**: Animation clip data structures
- **Core**: Error handling (Result<T>), telemetry schema

## Related Documentation

- [`ROADMAP.md`](ROADMAP.md): Module milestones including `RT-006` fuzzing initiative
- [`detection_fuzzing_playbook.md`](detection_fuzzing_playbook.md): Fuzzing workflows and signature database
- [`../../specs/ADR-0005-geometry-io-roundtrip.md`](../../specs/ADR-0005-geometry-io-roundtrip.md): IO architecture decisions
- [`../../tasks/T-0112-geometry-io-roundtrip-hardening.md`](../../tasks/T-0112-geometry-io-roundtrip-hardening.md): Hardening milestone


