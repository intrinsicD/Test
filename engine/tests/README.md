# Engine Test Suite

## Current State

- Collects unit, integration, and performance suites for all native subsystems.
- Integrates with CTest via the top-level CMake configuration.

## Usage

- Configure the project with `-DBUILD_TESTING=ON` and execute `ctest --test-dir build`.
- Add scenario coverage under the appropriate subdirectory as new features land.

## TODO / Next Steps

- Curate representative scenarios across unit, integration, and performance suites.
