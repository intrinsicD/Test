# CI Scripts

## Current State

- `run_presets.py` coordinates the canonical CMake preset matrix (Linux GCC + Windows MSVC) for smoke coverage.
- Script output mirrors CI logs and fails fast on any configure/build/test error.

## Usage

- Execute `./scripts/ci/run_presets.py` on the desired platform to build and test both debug and release presets.
- Pass `--platform windows` or `--platform linux` to force a specific toolchain; `--platform all` is available for aggregated runs.
- Use `--skip-release` when a fast debug-only validation is sufficient.

## TODO / Next Steps

- Integrate the preset runner into hosted CI once Windows agents are available.
- Capture additional diagnostics (artifacts, logs) once the pipelines move beyond smoke validation.
