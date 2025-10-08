# Engineering Notes — README Reconnaissance

_Date: 2025-02-14_

## Context
- Reviewed the repository root `README.md` in detail to extract build, testing, and layout guidance.

## Open Questions / Gaps
1. **Toolchain specificity** — The build section enumerates the CMake invocation but omits required compiler versions, SDK dependencies (e.g., Vulkan SDK), or minimum CMake version. Need clarification to guarantee reproducible builds across platforms.
2. **Python environment** — `pytest` is referenced for validating `engine3g.loader`, yet there is no description of the expected virtual environment, required packages, or how the Python bindings are generated. Determine whether `pip install -r requirements.txt` (currently absent) is necessary.
3. **Third-party updates** — The README claims certain libraries are vendored, but there is no policy for syncing upstream versions or documenting local patches. Establish an update cadence and contribution workflow.
4. **Testing scope** — C++ tests are invoked through `ctest`, but the README does not list the existing test suites nor whether coverage thresholds or sanitizers are enforced. Need to align on quality gates.
5. **Feature roadmap prioritisation** — The TODO backlog is exhaustive but unordered. Need product guidance to understand which features are in-flight for the current milestone.

## Next Steps
- Schedule a sync with build/release owners to enumerate minimum supported toolchains and platform-specific caveats.
- Draft a Python environment setup guide (potentially under `python/README.md`) once dependencies are confirmed.
- Propose documentation updates detailing third-party maintenance responsibilities.
- Audit existing tests to classify by subsystem and recommend coverage metrics.
- Request product/tech-lead input on TODO sequencing for roadmap planning.
