# Tests

_Path: `engine/tests`_

_Last updated: 2025-02-14_


## Overview

The `engine/tests/` subtree reserves space for cross-module test harnesses. While the current focus is on the
module-specific suites that live beside their respective code (e.g., `engine/math/tests`), these directories outline how
larger-scale testing will evolve:

- `unit/` – Aggregates focused unit tests that span multiple engine modules.
- `integration/` – Targets end-to-end scenarios such as loading scenes through the runtime façade.
- `performance/` – Hosts micro-benchmarks and performance regression harnesses.

Each directory contains a stub README and `.gitkeep` placeholder to aid future expansion. When promoting tests into this
hierarchy ensure they are registered with CTest inside the corresponding `CMakeLists.txt`.
