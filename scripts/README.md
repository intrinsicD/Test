# Developer Scripts

Automation helpers for building, validating, and packaging the workspace live under `scripts/`.

## Module Purpose

- `build/` – Shell and Python helpers that wrap common build invocations.
- `ci/` – Continuous integration entry points for linting, building, and testing in automation environments.
- Top-level scripts – Ad-hoc utilities such as documentation validators.

## Dependencies

- Python 3.12+
- CMake and Ninja when invoking build wrappers.
- Additional tooling specified in the individual scripts (e.g., environment variables for CI).

## Setup

No installation step is required beyond ensuring Python and the build toolchain from the root README are available on
your `PATH`.

## Build/Test Commands

- `python scripts/validate_docs.py` – Validates Markdown links across `docs/`.
- See `scripts/build/README.md` and `scripts/ci/README.md` for subsystem-specific commands.

_Last updated: 2025-02-14_
