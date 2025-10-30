# Example Configurations

## `ai004_sample.json`

`ai004_sample.json` demonstrates the AI-004 configuration schema end to end. It
references the `assets/datasets/remesh_sample` manifest, configures the research
rendering baseline, and enables telemetry capture compatible with the
prototyping harness and sandbox UI.

### Structure

- **`metadata`** – Declares the schema identifiers and version headers required
  by the AI-004 loaders (`ai-004.configuration`, `ai-004.dataset`,
  `ai-004.runtime`, `ai-004.rendering`).
- **`datasets`** – Imports `remesh_sample`, including asset integrity metadata
  (byte sizes, SHA-256 hashes, and provenance tags).
- **`runtime`** – Selects the dataset slug, scene manifest, timestep defaults,
  and telemetry template overrides used by the harness.
- **`rendering`** – Configures the research baseline preset, shading mode, and
  resolution forwarded to the runtime sample and sandbox UI.
- **`benchmarks`** – Captures a canonical scenario (`geometry-baseline`) so the
  comparative automation pipeline (CC-310) can drive headless runs.

### Validation

Run the validator whenever the manifest changes to catch schema regressions
early:

```bash
python -m scripts.validate_ai004_config \
    --dataset assets/datasets/remesh_sample/manifest.yaml \
    --config docs/examples/ai004_sample.json
```

### Harness workflows

Use the configuration with the Python CLI for dry-run validation, headless
execution, and metadata exports:

```bash
python -m scripts.prototyping.run_prototype_harness \
    --config docs/examples/ai004_sample.json \
    --dry-run --require-schema \
    --summary-json telemetry/ai004_dry_run.json
```

Combine `--describe-json` or `--case-studies-json` to surface summaries for the
TL-210 sandbox UI before launching the native runtime.

The native sample `engine/runtime/runtime_prototype_harness` accepts the same
configuration; see `engine/runtime/samples/README.md` for build instructions and
integration smoke tests.
