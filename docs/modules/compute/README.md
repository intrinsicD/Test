# Compute Module

## Current State
- Provides `KernelDispatcher` capable of measuring per-kernel execution time and
  reporting backend availability.
- Exposes backend capability probes to determine CUDA/CPU availability and a
  lightweight math helper for identity transforms.
- Dispatcher integrates with runtime job orchestration and publishes telemetry
  consumed by diagnostics tooling.

## Usage
- Build with `cmake --build --preset <preset> --target engine_compute`.
- Include `<engine/compute/dispatcher.hpp>` to register kernels and dispatch
  work; use `<engine/compute/api.hpp>` for backend queries.
- Configure builds with/without CUDA via `-DENGINE_ENABLE_CUDA`.
- Run `ctest --preset <preset> --tests-regex engine_compute`.

## TODO / Next Steps

- Track `CO-141`, `CO-150`, `CO-160` in the [central roadmap](../../ROADMAP.md) and update the execution checklist below when status changes — aligns with `AI-004` and `DC-002`.

This module tracks actionable work through the execution checklist below.

## Execution Checklist

| Task ID | Scope | Exit Criteria | Status |
| --- | --- | --- | --- |
| `CO-141` | Document dispatcher extension points (`AI-004`). | Publish guide describing kernel registration, dependency tracking, and telemetry hooks. | 🔄 In Progress |
| `CO-150` | Introduce dependency cycle detection. | Implement static analysis detecting kernel dependency loops with unit tests. | 🟢 Todo |
| `CO-160` | Align CUDA feature flags with presets. | Update presets/scripts ensuring CUDA optionality documented and tested. | 🟢 Todo |

See [ROADMAP.md](ROADMAP.md) for scheduling details.
