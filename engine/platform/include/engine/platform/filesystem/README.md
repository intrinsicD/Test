# Engine Platform Filesystem

## Current State

- Exposes sandboxed filesystem accessors and a virtual filesystem aggregator
  used to combine multiple mount points under stable aliases.

## Usage

- Use `engine::platform::filesystem::Filesystem` to perform read-only queries
  and file loads relative to a declared root directory.
- Mount the providers into `engine::platform::filesystem::VirtualFilesystem`
  to compose in-memory and on-disk assets under logical prefixes such as
  `"assets:/"`.

## TODO / Next Steps

- Extend the API with write/streaming capabilities and directory iteration
  helpers to support tooling workflows.
