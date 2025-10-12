# Core Module Roadmap

## Near Term
- Flesh out configuration loading by defining layered config sources (defaults, file overrides, command-line) and expose them through a stable API consumed by runtime.
- Implement diagnostics logging bridges that wrap spdlog sinks and integrate ImGui-based visualisation for ECS/system metrics.

## Mid Term
- Stand up the plugin subsystem to discover shared libraries, validate compatibility, and manage deterministic shutdown sequencing.
- Provide an extensible job/system scheduler API coordinating frame stages, enabling other modules to register update hooks.

## Long Term
- Harden multithreading support (component storage policies, registry snapshots) and document best practices for subsystem authors, backed by concurrency tests.
