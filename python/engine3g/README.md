# Python Engine Bindings

## Current State

- Contains the Python loader that discovers and interacts with compiled engine modules.
- Establishes the namespace for higher-level scripting utilities.
- Provides a `.pyi` stub for `engine3g.loader` so downstream tooling can adopt
  typed surfaces ahead of compiled binding coverage.

## Usage

- Ensure the engine shared libraries are discoverable before importing this package. The loader searches explicit overrides from
  `ENGINE3G_LIBRARY_PATH`, the package directory, the current working directory, and finally defers to the platform dynamic
  loader search path. Configure `ENGINE3G_LIBRARY_PATH` when libraries live outside those defaults.
- Explicit search paths passed to the loader can be provided as strings or `pathlib.Path` instances; the helper normalises them
  before probing candidates so custom directories work with either form.
- Module loaders accept either canonical identifiers (for example `engine_geometry`) or shorthand names (`geometry`,
  `rendering.core`). Both forms resolve to the same shared library prefix to simplify runtime integration scripts.
- `EngineRuntimeHandle.load_modules()` validates that each module name reported by the runtime is unique and aborts with a
  descriptive error if duplicates appear, preventing partially constructed module maps when subsystem registration is
  misconfigured.
- `EngineModuleHandle.compatibility_metadata()` parses JSON metadata exported by modules before the runtime initialises, allowing
  diagnostics to flag ABI or build skew without touching subsystem state.
- `EngineRuntimeHandle.loop_plan_serialization()` returns the JSON description of the active runtime loop plan, mirroring the
  native `engine_runtime_loop_plan_serialization()` export so harnesses can confirm stage ordering during RT-410 validation.
- `EngineRuntimeHandle.presentation_stage_active()` mirrors the native
  `engine_runtime_presentation_stage_active()` helper. Use it to guard screenshot capture,
  overlay composition, or other presentation-dependent scripting flows when running the hybrid
  harness in headless CI environments.
- `EngineRuntimeHandle` implements the context manager protocol. Use `with loader.load_runtime() as runtime:` to automatically
  initialise the runtime on entry and shut it down on exit. When the runtime was already initialised, the context manager
  preserves the existing lifetime so shared handles remain valid.
- Prefer `loader.runtime_session()` when you need a typed helper that manages the runtime lifetime and optionally preloads
  modules. The helper freezes any iterable search path input so generators or other one-shot iterables can be reused for both
  runtime and module loading. The yielded `RuntimeSession` exposes `.runtime` for direct access, `.modules` for inspection, and
  `.module(name)` to retrieve a specific `EngineModuleHandle` while reusing the harness tick helper.
- Load failures now raise `EngineLibraryNotFound` with structured context. Inspect `identifier`, `library_name`, and
  `attempted_paths` to surface actionable diagnostics in CLI wrappers or UI layers.
- Add ergonomic wrappers or CLI entry points alongside new runtime capabilities.
- `PrototypeHarness.interactive_session()` exposes a context-managed runtime controller for TL-210 and other interactive
  clients, enabling overlay toggles, deterministic tick control, and telemetry snapshots without reinitialising the harness.
- `PrototypeHarness.run_headless()` reports the runtime presentation state in its summary payload
  (`presentation_stage_active` and `loop_plan_serialization`) so PM-510 demos and automation can
  confirm stage planner wiring before recording captures.
- `HarnessExecutionOptions` accepts paired `resolution_width`/`resolution_height` overrides that surface through both the Python
  CLI (`--resolution-width/--resolution-height`) and the sandbox UI, allowing automated runs to enforce deterministic capture
  resolutions independent of persisted preferences. Provide both dimensions together to avoid aspect ratio ambiguity.
- Case study helpers (`case_studies.available_case_studies` and
  `case_studies.describe_case_studies`) accept an `include_tags` filter so tooling
  can surface module-specific scenarios without reimplementing registry parsing.

### Module Provenance

- Use `loader.collect_module_provenance(modules.values())` to expand module
  metadata into a dependency-aware provenance graph. Each record captures the
  resolved module name, the raw compatibility metadata exported by the shared
  library, and a recursively nested set of dependency entries annotated with
  resolution status, cycle detection, and requested constraints. Downstream
  tooling can project these structures directly into diagnostics dashboards to
  surface ABI incompatibilities or missing modules without re-querying native
  libraries.

## TODO / Next Steps

- Integrate the aggregated module provenance data into diagnostics viewers so
  dependency cycles and missing modules surface alongside existing telemetry.
