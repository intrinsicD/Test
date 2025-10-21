# AS-320 — Material Persistence Strategy

_Last Updated: 2025-03-24_

## Summary
Materials must persist consistently across authoring tools, on-disk assets, and the runtime caches that provide handles to
rendering backends. This design defines the canonical material document schema, outlines how the assets module serialises and
hydrates materials, and specifies hot-reload plus async streaming behaviours needed by `AI-002` and `CC-002`.

## Goals
- Provide a deterministic material document format with forward-compatible versioning.
- Support parameterised shader bindings (textures, samplers, uniform blocks, constants) with authoring metadata.
- Allow dependency tracking so material loads schedule prerequisite shader/texture loads automatically.
- Integrate with existing generational handle caches and async queue semantics, including filesystem-driven hot reload.
- Surface validation, logging, and telemetry needed to diagnose persistence or reload failures.

## Non-Goals
- Defining shader authoring workflows or graph-based material editors.
- Implementing GPU-side serialization of pipeline states (handled by rendering module).
- Shipping production-grade compression or encryption.
- Specifying UI tooling for editing materials beyond schema guidelines.

## Requirements
### Functional
1. Persist materials as standalone documents (`.material.json`) stored alongside referenced assets.
2. Encode bindings for:
   - Program/shader handle (vertex + fragment stages, plus optional compute).
   - Named texture slots (2D, cube, array) with sampler configuration references.
   - Structured parameters (uniform buffer blocks) and scalar overrides.
   - Render state overrides (blend, depth/stencil, raster state toggles) required for parity with runtime submission.
3. Support optional metadata blocks for authoring notes, tags, and dependency version pins.
4. Allow referencing parameter templates (inheritance) while keeping final resolved values explicit post-merge.
5. Expose deterministic ordering to avoid diff churn (lexicographically sorted keys, stable array ordering).

### Non-Functional
1. Round-trip serialisation must preserve values with float tolerances ≤ `1e-6`.
2. Runtime deserialisation must fail fast with actionable `AssetLoadError` codes.
3. Hot reload must debounce rapid writes and only rebind handles after dependent assets successfully reload.
4. Async streaming must account for dependency fan-out without deadlocking queue workers.
5. Document validation must execute within 0.5 ms per material for typical desktop targets (≈ 16 bindings).

## Material Domain Model
A material descriptor is normalised into the following structures inside the assets module:

```text
MaterialDocument
├─ version: SemanticVersion (major.minor.patch)
├─ metadata: MetadataBlock
├─ shader_program: ShaderBinding
├─ textures: Map<TextureSlot, TextureBinding>
├─ samplers: Map<SamplerSlot, SamplerBinding>
├─ parameters: Map<ParameterName, ParameterValue>
├─ uniform_blocks: Map<BlockName, UniformBlock>
├─ render_state: RenderStateOverrides
├─ dependencies: DependencyList
└─ variants: Map<VariantName, VariantOverrides>
```

- **ShaderBinding** — references vertex/pixel shader asset handles plus optional specialisation constants. Stores pipeline layout
  compatibility hash for validation against rendering backend metadata.
- **TextureBinding** — references texture asset handle, colour space, usage hints, and fallback strategy when unavailable.
- **SamplerBinding** — either references a sampler asset or embeds sampling parameters (filtering, wrapping, anisotropy).
- **ParameterValue** — typed union (float, int, bool, vecN, mat4, colour) storing values and UI hints.
- **UniformBlock** — raw byte payload plus reflection metadata hash to validate against shader layout.
- **RenderStateOverrides** — partial structure matching rendering module expectations (blend modes, depth compare, cull mode).
- **DependencyList** — resolved at load time into `AssetDependencyGraph` edges for async queue scheduling.
- **VariantOverrides** — optional map enabling LODs or platform-specific tweaks while sharing base definition.

## Persistence Format
### File Naming & Layout
- Primary extension: `.material.json` for readability and tooling friendliness.
- Companion binary cache: `.material.bin` generated during build to store packed uniform buffers (optional, per preset).
- Each document starts with header:

```json
{
  "type": "engine.material",
  "version": "1.0.0",
  "metadata": { ... },
  "shader_program": { ... },
  ...
}
```

- Keys are emitted in lexical order. Arrays (e.g., texture slots) are sorted by slot name.

### Schema Highlights
```json
{
  "metadata": {
    "name": "paint/gloss",
    "tags": ["automotive", "pbr"],
    "author": "tools@studio",
    "source_file": "materials/paint.gloss.material.json",
    "revision": 8,
    "last_modified_utc": "2025-03-24T09:21:35Z"
  },
  "shader_program": {
    "program": "shaders/pbr_forward.shader.json",
    "specialization": {
      "defines": {"USE_CLEARCOAT": true},
      "constants": {"MAX_LIGHTS": 4}
    },
    "layout_hash": "0x9c24af31"
  },
  "textures": {
    "albedo": {"texture": "textures/paint_albedo.ktx2", "srgb": true, "fallback": "textures/default_albedo.ktx2"},
    "normal": {"texture": "textures/paint_normal.ktx2", "srgb": false}
  },
  "samplers": {
    "default": {"wrap_u": "repeat", "wrap_v": "repeat", "min_filter": "linear_mipmap_linear", "mag_filter": "linear"}
  },
  "parameters": {
    "roughness": {"type": "float", "value": 0.15},
    "clearcoat_intensity": {"type": "float", "value": 0.6}
  },
  "uniform_blocks": {
    "MaterialUBO": {
      "binary": "@binary_cache/materials/paint.gloss.bin#MaterialUBO",
      "layout_hash": "0x8dd01243"
    }
  },
  "render_state": {
    "blend": {"mode": "alpha", "src": "src_alpha", "dst": "one_minus_src_alpha"},
    "depth": {"test": true, "write": true, "compare": "less"},
    "raster": {"cull_mode": "back", "front_face": "ccw"}
  },
  "dependencies": [
    {"type": "texture", "path": "textures/paint_albedo.ktx2"},
    {"type": "texture", "path": "textures/paint_normal.ktx2"},
    {"type": "shader", "path": "shaders/pbr_forward.shader.json"}
  ],
  "variants": {
    "mobile": {
      "shader_program": {"program": "shaders/pbr_mobile.shader.json"},
      "parameters": {"clearcoat_intensity": {"type": "float", "value": 0.0}}
    }
  }
}
```

### Versioning Strategy
- Semantic version increments:
  - `major` when schema changes are incompatible (requires migration tool).
  - `minor` for additive fields.
  - `patch` for validation tightening.
- Loader validates `type` and `version`. Unknown major versions produce `AssetLoadError::UnsupportedVersion`.
- Migration hooks registered per version allow transparent upgrades during load with telemetry logging.

## Data Flow & Runtime Integration
1. **Authoring** — Tools emit `.material.json` using schema library; optional `.material.bin` generated for uniform packs.
2. **Import** — Assets module `MaterialCache::load` parses JSON via `engine::io::JsonDocument`, validating against schema
   definitions stored in `engine::assets::MaterialSchemaRegistry`.
3. **Dependency Scheduling** — On load, dependencies register with `AssetDependencyGraph`. Async queue ensures prerequisite
   textures/shaders load before finalising the material; failures propagate structured errors.
4. **Handle Binding** — Once dependencies resolve, `MaterialCache` allocates pool slot, binds handle, and publishes callbacks.
5. **Runtime Submission** — Rendering module consumes material handles; metadata includes resolved resource IDs, uniform blocks,
   and render state overrides.
6. **Hot Reload** — Filesystem watcher invalidates material document. Cache enqueues reload; dependency graph ensures updated
   textures/shaders reload prior to rebinding. Observability metrics record reload latency and failure counts.

## Validation & Error Handling
- JSON schema validated using `engine::io::JsonSchemaValidator`; errors map to `MaterialLoadError` with context (path, field).
- Ensure referenced assets exist by probing corresponding caches; missing dependency yields `AssetLoadError::MissingDependency`.
- Uniform block hashes cross-check shader reflection; mismatches trigger `AssetLoadError::LayoutMismatch` and abort reload.
- Render state overrides validated against rendering backend capabilities; unsupported combinations logged as warnings and
  fallback to defaults.

## Performance & Scaling
- Parsing overhead mitigated by caching `MaterialDocument` AST in memory-mapped blob for frequently used materials.
- Binary uniform caches avoid repeated parsing of large constant buffers.
- Async queue uses batched dependency loads to prevent head-of-line blocking.
- Hot reload employs 150 ms debounce window configurable via module settings.

## Observability
- Extend `AssetStreamingTelemetry` to include `material_reload_latency` histogram and `material_reload_failures` counter.
- Emit structured logs on load/reload with correlation IDs for dependency chains.
- Hook into diagnostics shell (`AS-330`) to surface most recent material reload status per asset.

## Security & Integrity
- Reject documents with external URI schemes; only relative project paths allowed.
- Enforce max string lengths and array counts to avoid allocation spikes.
- Verify uniform binary cache checksums before consumption.
- Support optional signature block referencing IO module signature database once `RT-006` lands.

## Testing Strategy
- Unit tests covering:
  - Successful round-trip serialization/deserialization for representative materials.
  - Validation failures (missing shader, bad enum, unsupported version, hash mismatch).
  - Dependency resolution ordering with async queue mocks.
  - Hot reload debounce behaviour and callback invocation.
- Integration tests exercising runtime submission path with reloaded material ensuring rendering module receives updated state.
- Documentation validation via `scripts/validate_docs.py` to ensure schema references remain live.

## Implementation Plan
1. **Schema & Infrastructure (Sprint M3)**
   - Implement `MaterialSchemaRegistry` and JSON validators.
   - Define structs in `engine/assets/material_document.hpp` with serialization helpers.
   - Add CMake targets for schema tests.
2. **Cache Integration (Sprint M3)**
   - Extend `MaterialCache` to parse documents, register dependencies, and bind handles.
   - Wire async queue dependency scheduling and telemetry metrics.
   - Add hot reload debounce controls.
3. **Tooling & Docs (Sprint M3–M4)**
   - Publish CLI utility `material_inspect` under `tools/` for validation and packing binaries.
   - Document workflows in `docs/modules/assets/README.md` and `docs/modules/rendering/README.md`.
   - Update diagnostics shell once `AS-330` begins.
4. **Future Enhancements (Post-M4)**
   - Introduce template inheritance authoring helpers.
   - Explore binary-only format for console builds while keeping JSON as authoring source.

## Open Questions
- Confirm whether sampler assets should live in a dedicated cache or remain embedded definitions.
- Align render state override schema with pending rendering backend parity checklist (`RE-530`).
- Decide if materials require localisation support for metadata fields in future tooling.
