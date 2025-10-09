# Build Scripts

## Current State

- Hosts canonical CMake presets for Linux (GCC) and Windows (MSVC) builds.
- Provides toolchain files (`toolchains/`) that pin compiler selection and shared cache variables.
- Presets propagate GLFW and testing cache variables so FetchContent fallbacks match vendored builds.

## Usage

- Configure with `cmake --preset <preset>` (`linux-gcc-debug`, `linux-gcc-release`, `windows-msvc-debug`, `windows-msvc-release`).
- Build and test with matching build/test presets or `scripts/ci/run_presets.py` to exercise the full matrix.
- Extend `presets/*.json` and `toolchains/*.cmake` when onboarding new compilers or SDK toggles.

## TODO / Next Steps

- Expand the preset matrix to cover Clang/LLVM and macOS toolchains once dependencies are available.
- Automate dependency bootstrapping (e.g., Vulkan SDK detection) alongside preset selection.
