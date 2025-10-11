# Contributor Guidance

## 1. Always Start With the Documentation
- Read the repository root `README.md` before making changes; it summarises the workspace layout, build presets, and current TODO backlog.
- When touching any module, review its local `README.md`. Create or update it using `docs/README_TEMPLATE.md` so every directory explains:
  - What the component does and how it relates to neighbouring modules.
  - How to build and run its samples/tests.
  - Its local TODO items and how they map back to the aggregated backlog table in the root README.
- Keep documentation in sync with the implementation. Whenever behaviour, dependencies, or workflows change, update the relevant README(s) and design notes under `docs/`.

## 2. Environment & Dependencies
- **Compilers**: Use a modern C++20 toolchain (MSVC 19.3x, Clang ≥22, or GCC ≥12). Prefer Clang 22 when compiling locally.
- **CMake & Generators**: Require CMake ≥3.20. Use the provided presets (`cmake --preset <name>`) which default to Ninja generators. Add new presets under `scripts/build/` when new configurations are needed.
- **Python**: Use Python ≥3.12 with `pip`. Manage dependencies in a virtual environment and keep `requirements.txt` up to date.
- **System Packages (Linux/GLFW)**: Install `libxrandr-dev`, `libxinerama-dev`, `libxcursor-dev`, and `libxi-dev` alongside their runtime counterparts. Verify `xrandr` itself is available.
- **Graphics SDKs**: Install the relevant GPU backend SDKs (e.g., Vulkan SDK, DirectX 12 Agility SDK, or platform OpenGL drivers) before enabling those build options.
- Document any additional dependency you introduce in the appropriate README and, if necessary, automate its installation in `scripts/`.

## 3. Build & Run Workflow
1. Configure using the desired preset:
   ```bash
   cmake --preset linux-gcc-debug          # or another preset defined under scripts/build/
   ```
2. Build the project:
   ```bash
   cmake --build --preset linux-gcc-debug
   ```
3. Execute the C++ test suites:
   ```bash
   ctest --preset linux-gcc-debug
   ```
4. For Python tooling:
   ```bash
   python -m venv .venv
   source .venv/bin/activate
   pip install -r requirements.txt
   pytest
   ```
   Ensure `ENGINE3G_LIBRARY_PATH` points to the directory containing the built shared libraries before invoking Python loaders.
5. Validate documentation before submitting:
   ```bash
   python scripts/validate_docs.py
   ```
6. Extend CI orchestration via the scripts in `scripts/ci/` when you add new presets or automation paths.

## 4. Coding & Testing Standards
- Follow the conventions in `CODING_STYLE.md` (modern, high-performance C++20+). Do not wrap imports in try/catch blocks. Match the existing naming, formatting, and error-handling patterns.
- Comment complex algorithms with *why* and *how* rationales, not just *what*. Leverage docstrings or Doxygen-style comments for public headers.
- Every new feature must include accompanying unit/integration tests inside the relevant `engine/<module>/tests/` directory and be wired into CTest.
- Keep the aggregated TODO table in `README.md` synchronised with per-module README TODO sections.

## 5. Submission Checklist
- [ ] All affected README files updated to describe the current behaviour, dependencies, and TODO items.
- [ ] Required dependencies documented and, when possible, install scripts updated.
- [ ] Code adheres to `CODING_STYLE.md` and is sufficiently commented.
- [ ] `cmake --build`, `ctest`, Python tests, and `scripts/validate_docs.py` executed for the relevant presets; record the commands and results.
- [ ] No warnings or regressions introduced in existing tests.

Keep this guidance up to date as workflows evolve so newcomers can build, test, and extend the engine without surprises.
