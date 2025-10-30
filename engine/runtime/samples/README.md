# Runtime Harness Samples

## `runtime_prototype_harness`

The `runtime_prototype_harness` executable mirrors the Python prototyping
harness and exercises the AI-004 configuration schema directly from C++.
It accepts the same manifests consumed by the Python CLI, validates dataset
integrity, and records execution summaries for benchmark automation.

### Build

Use the standard presets to configure and build the sample alongside the
runtime module:

```bash
cmake --preset linux-gcc-debug
cmake --build --preset linux-gcc-debug --target runtime_prototype_harness
```

The target is emitted to
`out/build/linux-gcc-debug/engine/runtime/runtime_prototype_harness`
(or the equivalent directory for the selected preset).

### Dry-run validation

Validate manifests without starting the runtime by reusing the shipped sample
configuration:

```bash
./out/build/linux-gcc-debug/engine/runtime/runtime_prototype_harness \
    --config docs/examples/ai004_sample.json \
    --dry-run --require-schema \
    --summary-json telemetry/runtime_sample_summary.json
```

`--dry-run` enforces schema validation and dataset integrity checks while
skipping runtime initialization. `--summary-json` writes a telemetry payload
containing the resolved dataset slug, rendering preset, average tick time, and
kernel dispatch telemetry so CI automation can diff runs without parsing logs.

### Headless execution

Remove `--dry-run` to execute a fixed-timestep loop. Override `--frames` and
`--dt` to tune simulation length and cadence. The harness respects the
AI-004 telemetry template placeholders, so repeated executions emit unique
artefacts (for example `{dataset}`, `{scenario}`, `{run_index}`).

### Integration smoke coverage

CI exercises the executable via CTest. After configuring with the desired
preset, invoke:

```bash
ctest --preset linux-gcc-debug -R runtime_prototype_harness_sample_dry_run
```

The test launches the harness in dry-run mode with schema enforcement enabled
and verifies that the exported telemetry summary matches expectations. See the
Python-backed case study test (`runtime_prototype_harness_geometry_case_study`)
for additional coverage. Consult `docs/examples/README.md` for manifest details
and additional harness workflows.
