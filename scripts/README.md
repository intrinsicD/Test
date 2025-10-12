# Automation Scripts

## Current State

- Aggregates build presets (`build/presets`), toolchains (`build/toolchains`), and CI drivers (`ci/run_presets.py`).
- `CMakePresets.json` includes the preset fragments so standard `cmake --preset` invocations work from the repository root.

## Usage

- Run scripts from the repository root to maintain consistent relative paths.
- Use `cmake --preset <name>` for day-to-day configuration/build/test flows.
- Invoke `./scripts/ci/run_presets.py` in CI and local smoke runs to mirror the documented workflow.
- Run `python scripts/update_agents_tree.py` after layout changes to refresh the generated hierarchy in `AGENTS.md`.

## TODO / Next Steps

- Capture shared automation entry points for developers and CI environments.
- Document additional scripts as new workflows come online (packaging, deployment, etc.).
