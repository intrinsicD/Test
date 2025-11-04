# CC-310 Benchmark Automation Playbook

**Status:** ✅ Maintained — aligns with `CC-310`/`CC-311` deliverables.

This playbook describes the comparative benchmark automation pipeline used for
AI-004. It complements the `run_comparative_benchmarks.py` orchestrator,
`run_comparative_smoke.py` CI helper, and the telemetry viewer's comparative
reporting mode.

---

## 1. Artefact Overview

| Artefact | Purpose |
| --- | --- |
| `scripts/benchmarks/run_comparative_benchmarks.py` | Executes declarative benchmark scenarios, evaluates thresholds, and emits JSON/CSV/plot artefacts. |
| `scripts/ci/run_comparative_smoke.py` | Seeds the orchestrator with fixture metrics to validate configuration/summary generation in CI. |
| `assets/benchmarks/ai004/comparative_config.json` | Canonical configuration driving both smoke tests and published reports. |
| `assets/benchmarks/ai004/data/` | Fixture metrics per RT-321 case study (geometry baseline & rendering debug). |
| `assets/benchmarks/ai004/reports/` | Published summary JSON/CSV, SVG plots, and HTML report consumed by documentation and tooling. |
| `scripts/diagnostics/telemetry_viewer.py compare` | Renders HTML reports (with optional inline SVG) from orchestrator summaries. |

---

## 2. Running the Orchestrator

1. Populate a configuration matching the schema below (JSON or YAML):
   ```json
   {
     "version": 1,
     "output_directory": "{config_dir}/reports",
     "scenarios": [
       {
         "name": "geometry-baseline",
         "dataset": "geometry-remesh-baseline",
         "engine": {"command": null, "output": "{output_dir}/{scenario}_engine.json"},
         "reference": {"command": null, "output": "{output_dir}/{scenario}_reference.json"},
         "metrics": [
           {"name": "fps", "higher_is_better": true, "threshold": {"type": "relative", "max_regression": 0.02}}
         ]
       }
     ]
   }
   ```
   - `{output_dir}` and `{scenario}` placeholders resolve relative to the config file.
   - `command` tokens are optional; omit them when pre-seeding metric outputs for dry runs.
2. Execute the orchestrator:
   ```bash
   python scripts/benchmarks/run_comparative_benchmarks.py \
       --config assets/benchmarks/ai004/comparative_config.json \
       --output assets/benchmarks/ai004/reports/comparative_summary.json \
       --table assets/benchmarks/ai004/reports/comparative_summary.csv \
       --plot-dir assets/benchmarks/ai004/reports/plots \
       --dry-run
   ```
   - Omit `--dry-run` once runtime binaries are available and metric emitters are configured.
   - Use `--no-plots` to disable SVG generation for lightweight smoke executions.
3. Outputs:
   - `comparative_summary.json`: machine-readable results including per-metric plot references.
   - `comparative_summary.csv`: tabular summary for spreadsheets/BI tools.
   - `plots/*.svg`: engine vs. reference bar charts (one per metric).

---

## 3. CI Smoke Validation (`CC-310` DoD #3)

Run the smoke helper as part of CI and local pre-push hooks:
```bash
python scripts/ci/run_comparative_smoke.py --workspace-root .
```
- Copies fixture metrics from `assets/benchmarks/ai004/data/` into a temp workspace.
- Executes the orchestrator in `--dry-run` mode, ensuring schema + threshold evaluation remain healthy.
- Generates JSON/CSV/SVG outputs under the temporary directory and exits non-zero if regressions breach thresholds (>2%).

Integrate the command in CI after harness smoke tests so comparative gating stays under five minutes.

---

## 4. Comparative Report Generation (`CC-311` DoD #1/#2)

Transform orchestrator summaries into shareable HTML reports:
```bash
python scripts/diagnostics/telemetry_viewer.py compare \
    --summary assets/benchmarks/ai004/reports/comparative_summary.json \
    --output assets/benchmarks/ai004/reports/comparative_report.html \
    --plots-root assets/benchmarks/ai004/reports \
    --embed-plots
```
- `--embed-plots` inlines the generated SVGs, producing a self-contained artefact suitable for docs and sandbox previews.
- Without `--embed-plots`, the report links to SVGs via paths relative to the output directory (retain directory structure when publishing).

Published assets:
- `assets/benchmarks/ai004/reports/comparative_summary.json`
- `assets/benchmarks/ai004/reports/comparative_summary.csv`
- `assets/benchmarks/ai004/reports/plots/*.svg`
- `assets/benchmarks/ai004/reports/comparative_report.html`

These outputs cover both RT-321 case studies and are linked from the case-study design notes.

---

## 5. Troubleshooting & Best Practices

- **Missing plots**: ensure `plot` entries in the summary are relative to the summary directory and that `--plot-dir` is supplied when invoking the orchestrator.
- **Regression noise**: prefer relative thresholds for performance metrics (`fps`, `average_tick_ms`) and absolute thresholds for counts/time deltas sensitive to low baselines.
- **Extending metrics**: orchestrator metric payloads accept any numeric value. Keep metric names aligned with telemetry schema identifiers for traceability.
- **Documentation sync**: update this playbook, `docs/NAVIGATION.md`, and module READMEs whenever new scenarios or metrics land.

---

## 6. Checklist

- [x] Comparative config validated via `run_comparative_smoke.py`.
- [x] SVG plots generated for each metric (`plots/*.svg`).
- [x] HTML report regenerated and published (`telemetry_viewer.py compare`).
- [x] Backlog + roadmap updated (`CC-310`, `CC-311` set to *Complete*).

---

**Last updated:** 2026-02-20
