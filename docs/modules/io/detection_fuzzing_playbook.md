# IO Signature Detection & Fuzzing Playbook

## Purpose
Roadmap task `RT-006.3` delivers a repeatable playbook for IO signature
detection and fuzzing. The goal is to keep geometry and animation importers
predictable while new formats and datasets arrive. This guide explains how the
current detectors work, how to extend the signature catalogue, and how to
operate the fuzzing harnesses that guard the code paths.

## Scope & Prerequisites
- Covers the geometry and animation detection entry points:
  - `engine::io::detect_geometry_file` in
    [`engine/io/src/geometry_io.cpp`](../../../engine/io/src/geometry_io.cpp)
    controls mesh/point cloud/graph sniffing.
  - `engine::io::animation::detect_clip_format` in
    [`engine/io/src/animation_importer.cpp`](../../../engine/io/src/animation_importer.cpp)
    resolves clip formats before loading.
- Applies to the libFuzzer harness defined in
  [`engine/io/tests/geometry_io_detection_fuzz.cpp`](../../../engine/io/tests/geometry_io_detection_fuzz.cpp).
- Requires a C++20 toolchain with sanitizer + libFuzzer support when fuzzing.
  Pass `-DENGINE_ENABLE_FUZZING=ON` to `cmake` to build fuzz targets.
- Always run `python scripts/validate_docs.py` after editing this guide or
  linked READMEs to keep navigation intact.

## Detection Workflow

### Geometry Detection Pipeline
`detect_geometry_file(path)` emits a `GeometryDetectionResult` describing the
kind of asset, the candidate format, and a `format_hint` string. The detector
evaluates inputs in the following order:

1. **Extension classification** — `classify_extension_only` inspects the final
   extension (and nested stems) to seed `format_hint` and suggest mesh/point
   cloud/graph defaults. Use this as the fast path for supported extensions
   (`.obj`, `.ply`, `.stl`, `.pcd`, `.xyz`, etc.).
2. **PLY structural inspection** — when the extension resolves to `.ply`, the
   detector parses the header via `inspect_ply_header` to disambiguate meshes,
   graphs, and point clouds based on declared element counts. Failures surface
   as `GeometryIoError::invalid_argument` or `io_failure`.
3. **STL signature detection** — files with `.stl` (or unknown extensions) run
   through `detect_stl_from_signature` to handle ASCII/Binary STL without
   relying purely on extensions.
4. **Heuristic signatures** — `detect_geometry_from_signatures` reads the
   leading bytes/lines and invokes helpers such as `looks_like_off_signature`,
   `looks_like_obj_signature`, `looks_like_pcd_signature`, and
   `looks_like_edgelist_signature` to map unknown extensions back to supported
   formats.
5. **Fallback header checks** — if the above steps fail, the detector opens the
   file and inspects the first line for `ply`/`off` markers before returning an
   `unsupported_format` error when the heuristics cannot classify the input.

The detector never throws; instead it returns `GeometryIoResult<...>` instances
with structured error identifiers. Callers must log
`GeometryIoError::identifier()` alongside contextual metadata so telemetry and
triage dashboards can bucket failures deterministically.

### Animation Clip Detection
`animation::detect_clip_format(path)` mirrors the geometry flow but is purpose
built for clip JSON:

1. `classify_extensions` walks the last three stems to catch multi-part names
   such as `walk_idle.anim.json`.
2. If extensions fail, `sniff_json_signature` streams from disk until the first
   non-whitespace character. `{` or `[` marks JSON (`ClipFormat::json`), while
   other characters produce `ClipFormat::unknown` and, eventually,
   `AnimationIoError::unsupported_format` once `load_clip` is invoked.

Document error sources in task records (`T-0112`) when detection returns
unexpected identifiers; mismatches usually indicate either missing plugins or a
signature catalogue gap.

## Signature Catalogue Management

`RT-006.1` introduced a data-driven signature database consumed by the geometry
detector. Use this checklist whenever curating or updating the catalogue:

1. **Source samples** — collect minimal fixtures covering the new format/variant
   (ASCII vs binary, presence of comments, large headers). Store them under
   `engine/io/tests/fixtures/<format>/` with provenance notes.
2. **Record signatures** — encode prefix patterns (magic numbers, canonical
   tokens) inside `engine/io/signatures/geometry_signatures.json`. Include
   fields for `kind`, `format`, `offset`, and `pattern` so the detector can
   dispatch without new code. Select a `match.type` (`byte_prefix`,
   `line_prefix`, or `contains_all`) that minimises false positives.
3. **Validate locally** — run `ctest --preset <preset> --tests-regex
   engine_io` to exercise unit coverage. Add targeted unit tests when extending
   detection heuristics so regressions fail deterministically.
4. **Refresh documentation** — update this playbook and the IO README with the
   new format, listing required extensions and signature tokens.
5. **Commit provenance** — annotate catalogue entries with source repositories,
   licenses, and generator scripts to keep legal review straightforward.

Set the `ENGINE_IO_GEOMETRY_SIGNATURE_PATH` environment variable to point at an
alternate catalogue when experimenting with draft signatures or trimmed
datasets. Tests call `engine::io::detail::reset_geometry_signature_cache_for_testing()`
after changing the environment so the loader re-reads the override.

Until the catalogue lands, continue capturing manual heuristics in the code but
prepare fixture folders so the transition to data-driven detection remains
trivial.

## Fuzzing Playbook

### Build the Harness
1. Configure CMake with fuzzing enabled:
   ```bash
   cmake --preset <preset> -DENGINE_ENABLE_FUZZING=ON
   cmake --build --preset <preset> --target engine_io_geometry_fuzz
   ```
2. Sanitizers: GCC/Clang builds automatically inject `-fsanitize=fuzzer,address`
   when the preset honours the fuzzing option. For MSVC, rely on LLVM-based
   toolchains or run the standalone driver mode (`main` guarded by
   `ENGINE_IO_FUZZ_WITH_LIBFUZZER`).

### Seed Corpus Management
- Store canonical seeds under
  `engine/io/tests/corpus/geometry_detection/`. Maintain a README describing the
  intent of each seed and its provenance.
- Include edge cases: truncated headers, oversized comments, inconsistent
  counts, binary/ASCII variants, and deliberately malformed payloads that should
  fail gracefully.
- Keep the directory checked into source control; CI jobs sync it before
  invoking libFuzzer.

### Running libFuzzer
Run libFuzzer locally with an isolated workdir to avoid polluting the corpus:

```bash
./out/engine_io_geometry_fuzz \
    -artifact_prefix=artifacts/ \
    -max_total_time=300 \
    engine/io/tests/corpus/geometry_detection
```

Recommended flags:
- `-rss_limit_mb=4096` — prevent runaway allocations.
- `-use_value_profile=1` — improve coverage for format classifiers.
- `-dict=<path>` — once available, supply token dictionaries (e.g., `ply`,
  `OFF`, `solid`).

### Standalone Reproducer Mode
When libFuzzer is unavailable, use the harness' CLI entry point:

```bash
./out/engine_io_geometry_fuzz /path/to/sample.ply
```

This path executes a single iteration by writing the bytes to a temporary file
(`engine_io_detection_fuzz.tmp`) and calling `detect_geometry_file` plus the
follow-up import attempt.

### Crash & Regression Triage
1. Reproduce the failure with the saved artifact (`crash-*` or
   `timeout-*`). Run the harness in standalone mode for deterministic repros.
2. Inspect logs/telemetry to determine whether the crash stemmed from detection
   (`GeometryDetectionResult` still `unknown`) or from the importer invoked by
   the harness. Remember that the harness deliberately swallows importer
   exceptions to keep the focus on crashes/UB.
3. File a bug with the minimized corpus entry, stack trace, and
   `GeometryIoError` identifier (if any). Link to `RT-006` subtasks or create a
   new IO module ticket when the issue requires code changes.
4. Promote the reproducing file into the checked-in corpus once fixed so the
   regression remains covered.

## Observability & Reporting
- Instrument detection failures by logging the error identifier and the first
  64 bytes of the file (hashed) to avoid leaking asset contents.
- Wire metrics through `IO-240` once available so repeated detection failures
  surface in the diagnostics viewer (`CC-001`).
- Capture fuzzing runs in CI artefacts: keep the log, coverage report, and any
  crash reproducer. Coordinate with CI owners before turning fuzzing on by
  default because of the resource requirements.

## Maintenance Checklist
- [ ] Refresh the signature catalogue when new formats are onboarded (`RT-006.1`).
- [ ] Expand the fuzzing corpus quarterly with fixtures from production bugs.
- [ ] Rotate sanitizer toolchains annually to pick up libFuzzer improvements.
- [ ] Update this playbook and the IO README whenever detection or fuzzing
      workflows change.
- [ ] Mirror status updates into [`docs/ROADMAP.md`](../../ROADMAP.md) and the
      module roadmap (`docs/modules/io/ROADMAP.md`).

