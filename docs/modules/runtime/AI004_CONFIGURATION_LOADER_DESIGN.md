# Runtime AI-004 Configuration Loader — Design Summary

**Owner:** Tech Lead (Runtime)

**Scope:** Implement schema-aligned configuration loading and validation for the runtime harness as part of backlog item [DC-040](../../backlog/active/DC-040-ai-004-configuration-schema.md).

## Purpose

Provide a native C++ implementation of the AI-004 configuration/schema loader so the runtime harness consumes the same manifest contract as the Python tooling. The loader must:

- Parse YAML and JSON manifests describing datasets, rendering presets, runtime options, benchmarks, and telemetry.
- Enforce schema invariants (slug formatting, required fields, versioned metadata, and cross-references).
- Share enforcement gates with existing tooling (`ENGINE_AI004_SCHEMA_V1` feature flag and CLI overrides).
- Surface actionable validation errors via `engine::runtime::RuntimeErrorCode` without terminating the process.

## Interfaces

New header: `engine/runtime/include/engine/runtime/config_schema.hpp`

```cpp
namespace engine::runtime::config
{
    struct DatasetEntry { /* strongly typed fields */ };
    struct DatasetManifest { std::vector<DatasetEntry> datasets; };
    struct RenderingConfig { /* preset, shading mode, overlays */ };
    struct RuntimeConfig { /* dataset slug, scene manifest, camera, simulation */ };
    struct BenchmarkConfig { /* scenarios + metrics */ };
    struct TelemetryConfig { /* outputs, metrics, sampling */ };
    struct Ai004Configuration
    {
        DatasetManifest datasets;
        std::optional<RenderingConfig> rendering;
        std::optional<RuntimeConfig> runtime;
        std::optional<BenchmarkConfig> benchmarks;
        std::optional<TelemetryConfig> telemetry;
    };

    [[nodiscard]] RuntimeResult<DatasetManifest>
    load_dataset_manifest(const std::filesystem::path& path,
                          std::optional<bool> require_schema = std::nullopt) noexcept;

    [[nodiscard]] RuntimeResult<Ai004Configuration>
    load_configuration(const std::filesystem::path& path,
                       std::optional<bool> require_schema = std::nullopt) noexcept;
}
```

The loader returns `RuntimeResult<T>` so callers can decide how to surface validation failures. Errors use new `RuntimeError` variants (`configuration_io_error`, `configuration_parse_error`, `configuration_validation_error`).

## Data & Control Flow

1. **Read manifest:** load UTF-8 text from disk, erroring with `configuration_io_error` when inaccessible.
2. **Parse YAML:** use `yaml-cpp` (fetched via `third_party`) so both YAML and JSON manifests are supported. JSON is handled because YAML is a superset.
3. **Canonicalise schema headers:** when strict enforcement is disabled (no override + feature flag unset) inject default schema headers (`ai-004.*`, version `1`) before validating, matching Python behaviour.
4. **Validate & materialise:** traverse the parsed tree, mapping to strongly typed structs while enforcing invariants:
   - slugs: lowercase alphanumeric with hyphen separators
   - schema versions: dataset v2 requires SHA-256 + size metadata
   - enumerations: rendering shading modes, benchmark threshold types, telemetry statistics, etc.
   - cross references: runtime dataset + benchmark scenarios must reference declared datasets
5. **Return results:** on success, return populated structs; on validation failure, return `configuration_validation_error` with descriptive message for CLI/UI consumption.

## Memory & Performance Considerations

- Parsing is single-pass and uses stack-local helper functions; resulting structs own compact STL containers (`std::vector`, `std::array`, `std::optional`).
- Dataset parameterisation and benchmark sections store vectors rather than linked structures to stay cache friendly.
- YAML trees are discarded immediately after translation, keeping peak allocations modest (<1 MB for representative manifests).
- Loader executes during harness startup; target budget is ≈1 ms for typical manifests. `yaml-cpp` is sufficient for this footprint.

## Error Handling & Logging

- All public APIs return `RuntimeResult<T>`; callers can log using existing runtime logging utilities (`spdlog`).
- Validation helpers throw no exceptions; internal parsing exceptions from `yaml-cpp` are caught and wrapped into `RuntimeErrorCode` results.
- Feature flag evaluation mirrors Python: environment variable `ENGINE_AI004_SCHEMA_V1` accepts `1,true,on,yes,enable,enabled` (case-insensitive). CLI overrides map to the optional `require_schema` parameter.

## Testing Strategy

- **Unit tests (`engine/runtime/tests/test_config_schema.cpp`):**
  - Load legacy manifests without schema headers when enforcement disabled (defaults injected).
  - Verify schema enforcement via environment flag and override parameter.
  - Validate slug formatting, versioned metadata requirements, and cross-references (runtime dataset must exist).
  - Exercise rendering/runtime/benchmark/telemetry parsing using representative manifests.
- **Integration hooks:** Python harness already exercises schema logic; the runtime harness will call the new loader once integrated.
- **Docs:** Update `docs/design/AI-004-configuration-schema.md`, runtime module README, backlog, and roadmap entries to reflect native loader availability.

## Dependencies

- Add `yaml-cpp` via `third_party` FetchContent to provide a battle-tested YAML parser without bespoke parsers.
- Ensure build targets expose include directories and link the runtime module against `yaml-cpp::yaml-cpp`.

