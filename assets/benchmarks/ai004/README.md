# AI-004 Comparative Benchmark Artefacts

This directory stores declarative configurations, metric fixtures, and generated
reports for the AI-004 comparative benchmark workflow (`CC-310`/`CC-311`).

- `comparative_config.json` — canonical configuration consumed by
  `scripts/benchmarks/run_comparative_benchmarks.py`.
- `data/` — per-scenario metric fixtures used by CI smoke tests and report
  regeneration scripts.
- `reports/` — generated summaries, tables, plots, and HTML reports referenced
  by documentation and the sandbox UI.

Use `python scripts/ci/run_comparative_smoke.py` to validate the configuration
without rebuilding runtime binaries. To refresh the published artefacts, pass
`--output-dir assets/benchmarks/ai004/reports` so the orchestrator writes into
this directory.

Regenerate the HTML report via:

```bash
python scripts/ci/run_comparative_smoke.py --workspace-root .  # ensures fixtures are valid
python scripts/benchmarks/run_comparative_benchmarks.py \
    --config assets/benchmarks/ai004/comparative_config.json \
    --output assets/benchmarks/ai004/reports/comparative_summary.json \
    --table assets/benchmarks/ai004/reports/comparative_summary.csv \
    --plot-dir assets/benchmarks/ai004/reports/plots \
    --dry-run
python scripts/diagnostics/telemetry_viewer.py compare \
    --summary assets/benchmarks/ai004/reports/comparative_summary.json \
    --output assets/benchmarks/ai004/reports/comparative_report.html \
    --embed-plots
```

The dry-run leverages the fixtures in `data/` (copied automatically when running
the smoke helper with `--output-dir`) and produces reproducible outputs for
documentation.
