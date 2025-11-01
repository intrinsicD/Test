# Automation Scripts

## Current State

- Aggregates build presets (`build/presets`), toolchains (`build/toolchains`), and CI drivers (`ci/run_presets.py`).
- `CMakePresets.json` includes the preset fragments so standard `cmake --preset` invocations work from the repository root.
- Telemetry and diagnostics helpers live in [`diagnostics/`](diagnostics/); see
  [`diagnostics/README.md`](diagnostics/README.md) for the runtime frame timing
  capture script that fulfils the sprint 06 telemetry requirement.
- Error-handling lint (`lint/error_handling.py`) guards against reintroducing
  legacy exception patterns covered by `DC-004`.
- Comparative benchmarking entry point lives in
  [`benchmarks/run_comparative_benchmarks.py`](benchmarks/run_comparative_benchmarks.py);
  it executes engine and reference workloads from a declarative configuration
  and enforces regression thresholds for `CC-310`.
- AI-004 dataset ingestion lives in
  [`datasets/ingest_dataset.py`](datasets/ingest_dataset.py); it stages manifests
  into reproducible caches with checksum metadata and regression coverage to
  advance `AS-330` deliverables.
- Module dependency visualisation lives in [`generate_dependency_graph.py`](generate_dependency_graph.py); it emits Graphviz DOT/SVG to keep architecture docs synchronised.

## Usage

- Run scripts from the repository root to maintain consistent relative paths.
- Use `cmake --preset <name>` for day-to-day configuration/build/test flows.
- Invoke `./scripts/ci/run_presets.py` in CI and local smoke runs to mirror the documented workflow.
- Run `python scripts/update_agents_tree.py` after layout changes to refresh the generated hierarchy in `AGENTS.md`.
- Package dataset manifests into cache directories with
  `python -m scripts.datasets.ingest_dataset <manifest> --copy-assets`.
- Preview manifest metadata for packaging reviews using
  `python -m scripts.datasets.ingest_dataset <manifest> --dry-run --summary artifacts/datasets_summary.json`.
- Bootstrap the Python environment for contributors and CI workers with
  `python -m scripts.bootstrap_python_env` (implements roadmap item `PY-015`).
- Inspect bundled AI-004 case studies without executing the runtime by running
  `python -m scripts.prototyping.run_prototype_harness --list-case-studies`
  and optionally providing `--case-studies-json <path>` to export metadata for
  tooling integrations (`RT-320`, `TL-210`).
- Enumerate datasets declared in a configuration with
  `python -m scripts.prototyping.run_prototype_harness --config <path> --list-datasets`
  and optionally `--datasets-json <path>` to export the verification summary for
  tooling consumers (`AS-330`).

## TODO / Next Steps

- Capture shared automation entry points for developers and CI environments.
- Document additional scripts as new workflows come online (packaging, deployment, etc.).
- Integrate the runtime diagnostics JSON output into CI dashboards so
  regressions are tracked continuously.
