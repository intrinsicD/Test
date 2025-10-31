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
  (byte sizes, SHA-256 hashes, provenance summaries, and licensing details).
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

Override the dataset or rendering selection without editing the manifest using
the harness CLI options introduced for TL-210/CC-310 integration:

```bash
python -m scripts.prototyping.run_prototype_harness \
    --config docs/examples/ai004_sample.json \
    --dry-run --dataset remesh-variant --runtime-profile diagnostics \
    --rendering-preset diagnostics --shading-mode forward \
    --overlay normals=1 --overlay uv=0 \
    --summary-json telemetry/diagnostics.json
```

The native sample `engine/runtime/runtime_prototype_harness` accepts the same
configuration; see `engine/runtime/samples/README.md` for build instructions and
integration smoke tests.

List dataset metadata and verification status without executing the runtime by
invoking:

```bash
python -m scripts.prototyping.run_prototype_harness \
    --config docs/examples/ai004_sample.json \
    --list-datasets --datasets-json telemetry/dataset_inventory.json
```

The JSON export mirrors the sandbox configuration summary so dataset packaging
(`AS-330`) and tooling integrations can consume the same verification payload.

<a id="rendering_debug"></a>

## Rendering debug dataset

- **Manifest** – `assets/datasets/rendering_sample/manifest.json`
- **Use case** – Validate shading overlays and telemetry wiring in the research
  rendering baseline after ingesting tessellated quads.
- **Ingestion** –
  ```bash
  python -m scripts.datasets.ingest_dataset \
      assets/datasets/rendering_sample/manifest.json \
      --output artifacts/datasets --require-schema
  ```
- **Harness selectors** – Choose the `rendering-quad-shading` dataset when
  exercising CC-310 comparative runs that focus on overlay instrumentation.

<a id="animation_walk"></a>

## Animation walk dataset

- **Manifest** – `assets/datasets/animation_sample/manifest.json`
- **Use case** – Provide retargeted walk-cycle clips so runtime and sandbox
  flows can test animation harness integration without external assets.
- **Ingestion** –
  ```bash
  python -m scripts.datasets.ingest_dataset \
      assets/datasets/animation_sample/manifest.json \
      --output artifacts/datasets --require-schema
  ```
- **Harness selectors** – Use the `animation-walk-retarget` slug for animation
  smoke tests and telemetry recording scenarios.
