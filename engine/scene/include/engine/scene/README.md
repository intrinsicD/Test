# Engine Scene Public Headers

## Current State

- Exposes the stable public headers for scene entities, systems, serialization, and component definitions.
- Provides a single aggregation point (`components.hpp`, `systems.hpp`) to simplify downstream includes.

## Usage

- Include headers from `<engine/scene/...>` when consuming the public API.
- Keep header changes paired with updates to the module tests and documentation.

## TODO / Next Steps

- Document each public header (entry points, ownership rules, threading expectations) as the API hardens.
- Track ABI-impacting changes and reflect them in release notes for downstream tooling.
