# Engine Rendering Public Headers

## Current State

- Exposes public headers that mirror the module API and guard ABI compatibility.
- Publishes the `IGpuScheduler` interface and frame-graph contracts used by the runtime.

## Usage

- Include headers from `<engine/rendering/...>` when consuming the public API.
- Keep header changes paired with updates to the module tests and documentation.

## TODO / Next Steps

- Document the public headers once the API stabilises.
- Provide backend-specific scheduler documentation when hardware integrations land.
