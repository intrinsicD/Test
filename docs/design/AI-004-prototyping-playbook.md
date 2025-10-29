# AI-004 Prototyping Playbook

**Status:** ✅ Active

**Related Initiatives:** `DC-040`, `RT-320`, `TL-210`, `AS-330`, `CC-310`

---

## Purpose

This playbook documents the end-to-end workflow that powers the AI-004
prototyping stack. Follow it whenever you need to:

- Package or update configuration manifests that adhere to the shared schema.
- Curate datasets and validate provenance prior to ingestion.
- Launch the runtime prototyping harness or its sandbox UI counterpart.
- Capture benchmark artefacts and telemetry for comparative studies.

Use the checklist at the end of each stage to ensure hand-offs between teams
remain deterministic.

---

## 1. Prerequisites

1. **Dependencies** – Install Python 3.12 with `pip`, CMake ≥ 3.20, Ninja,
   and a C++20 toolchain (Clang 22, GCC 13, or MSVC 19.38).
2. **Python packages** – Install harness requirements:
   ```bash
   python -m venv .venv
   source .venv/bin/activate
   pip install -r python/requirements.txt
   ```
3. **Runtime library** – Build the runtime preset you plan to exercise:
   ```bash
   cmake --preset linux-gcc-debug
   cmake --build --preset linux-gcc-debug
   ```
4. **Environment flags** – During migration set
   `ENGINE_AI004_SCHEMA_V1=1` to require explicit schema headers.

---

## 2. Shared Configuration Schema

The schema lives in
[`docs/design/AI-004-configuration-schema.md`](AI-004-configuration-schema.md)
and is enforced by loaders in both Python and C++.

### 2.1 Authoring manifests

- Start from the sample configuration in
  [`docs/examples/ai004_sample.json`](../examples/ai004_sample.json).
- Run the validator before committing changes:
  ```bash
  python -m scripts.validate_ai004_config \
      --dataset assets/datasets/remesh_sample/manifest.yaml \
      --config docs/examples/ai004_sample.json
  ```
- When `ENGINE_AI004_SCHEMA_V1` is unset the loaders inject schema defaults so
  legacy manifests stay functional while teams migrate.

### 2.2 Enforcement gates

| Context | Enforcement strategy |
| --- | --- |
| CI smoke tests | Invoke the validator plus the harness with `--require-schema`. |
| Local iteration | Export `ENGINE_AI004_SCHEMA_V1=1` or pass `--require-schema`. |
| Sandbox UI | Reject manifest selections that fail validation and surface the error toast. |

**Checklist:** Schema headers declared, dataset slugs unique, manifests validated.

---

## 3. Dataset Packaging (`AS-330`)

1. Generate remeshing outputs or curated datasets.
2. Record metadata (source mesh SHA-256, byte sizes, telemetry statistics).
3. Update `assets/datasets/<name>/manifest.yaml` using the schema fields.
4. Capture provenance in the dataset README and link licensing artefacts.
5. Run:
   ```bash
   python -m scripts.validate_ai004_config --dataset assets/datasets/<name>/manifest.yaml
   ```
6. Store manifests and assets in Git LFS as appropriate.

**Checklist:** Metadata populated, validator clean, provenance captured, licensing reviewed.

---

## 4. Runtime Prototyping Harness (`RT-320`)

### 4.1 Dry runs

```bash
python -m scripts.prototyping.run_prototype_harness \
    --config docs/examples/ai004_sample.json \
    --dry-run --require-schema
```

The dry run parses the configuration, verifies dataset references, and prints a
summary without loading the native runtime library. Use it for CI smoke tests.

### 4.2 Headless execution

```bash
python -m scripts.prototyping.run_prototype_harness \
    --config docs/examples/ai004_sample.json \
    --frames 600 --dt 0.0166667 \
    --summary-json telemetry/ai004_run.json
```

- `--frames`/`--dt` control simulation cadence.
- `--summary-json` writes telemetry and benchmark metadata (including `average_tick_ms`) to disk.
- Provide `--case-study <id>` to resolve configurations from
  `assets/datasets/case_studies/` once they are published.

### 4.3 Troubleshooting

- `ConfigurationSchemaError`: re-run the validator to inspect the failing field.
- Missing datasets: ensure manifests declare unique slugs and the configuration
  references them correctly.
- Missing runtime library: build the runtime preset referenced by your harness
  invocation.

**Checklist:** Harness CLI executed with schema enforcement, summaries captured,
errors resolved via validator logs.

---

## 5. Experiment Sandbox (`TL-210`)

1. Launch the sandbox once the harness back-end is available:
   ```bash
   ./out/build/linux-gcc-debug/tools/experiment_sandbox \
       --config docs/examples/ai004_sample.json
   ```
2. Use dataset and preset selectors to drive the harness; the UI mirrors the
   schema-defined fields.
3. Persist layout and preferences on shutdown (stored in `$XDG_CONFIG_HOME`).
4. Export benchmark captures from the sandbox to feed comparative automation.

**Checklist:** UI booted with schema-compliant config, preferences saved,
benchmark actions produce harness summaries.

---

## 6. Benchmark Automation (`CC-310`)

1. Configure comparative scenarios in the `benchmarks` section of the schema.
2. Execute automation scripts:
   ```bash
   python scripts/diagnostics/compute_dispatch_benchmark.py \
       --sample ./out/build/linux-gcc-debug/engine/compute/engine_compute_runtime_sample \
       --runs 3 --frames 1024 --workload balanced --baseline \
       --output-dir telemetry/dispatch_benchmark
   ```
3. Archive summaries and visualisations for CI; future work will publish
   `CC-310` automation and `CC-311` dashboards building on the same configuration.

**Checklist:** Benchmarks reference valid datasets, outputs archived, regression
thresholds respected.

---

## 7. Migration Timeline

1. **Preview (now)** – Defaults auto-injected; update manifests opportunistically.
2. **Enforcement (upon RT-320 launch)** – `ENGINE_AI004_SCHEMA_V1` enabled by
   default in CI presets; harness rejects legacy manifests.
3. **Removal (post-RT-321)** – Legacy loaders deleted; docs updated to require
   schema headers.

Communicate schedule changes in `docs/ROADMAP.md` and module READMEs.

---

## 8. Troubleshooting Matrix

| Symptom | Resolution |
| --- | --- |
| `schema.id` mismatch | Ensure each section declares its canonical ID (e.g., `ai-004.runtime`). |
| Duplicate dataset slug | Remove duplicates; each slug must be unique across the manifest. |
| Sandbox launch fails | Run the dry-run CLI to confirm config validity, then check runtime library path. |
| Missing telemetry file | Verify `--summary-json` path and that the harness has write permissions. |
| Benchmark regression | Inspect telemetry summaries; rerun with `--baseline` to capture reference data. |

---

## 9. Command Reference

| Goal | Command |
| --- | --- |
| Validate manifests | `python -m scripts.validate_ai004_config --dataset <path> --config <path>` |
| Dry-run harness | `python -m scripts.prototyping.run_prototype_harness --dry-run --require-schema --config <path>` |
| Execute harness | `python -m scripts.prototyping.run_prototype_harness --frames N --dt 0.0166667 --summary-json <path> --config <path>` |
| Launch sandbox | `./out/build/<preset>/tools/experiment_sandbox --config <path>` |
| Run benchmark helper | `python scripts/diagnostics/compute_dispatch_benchmark.py --sample <binary> --runs 3 --frames 1024 --baseline` |

---

## 10. Handoff Checklist

- [ ] Schema headers declared and validated.
- [ ] Dataset manifests packaged with provenance.
- [ ] Harness dry run executed with `--require-schema`.
- [ ] Sandbox preferences saved after schema change.
- [ ] Benchmark outputs archived with configuration revision noted.

Keep this playbook in sync with `DC-040`, `RT-320`, `TL-210`, and `AS-330`
whenever schema fields or automation behaviour changes.
