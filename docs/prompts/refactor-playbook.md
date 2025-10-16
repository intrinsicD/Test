# Refactor Playbook

Use this prompt when guiding AI assistants through refactoring work in the Test Engine repository.

## When to Use
- Large structural changes affecting multiple modules
- Changes that modify architectural invariants
- Multi-file updates requiring coordination across subsystems
- Implementation of architecture improvement plan items (DC-*, AI-*, RT-*)

## Prerequisites Checklist
Before starting any refactor, verify:
- [ ] The change maps to a specific roadmap item in `docs/ROADMAP.md`
- [ ] Relevant ADRs exist in `docs/specs/` or a new one is planned
- [ ] Affected modules and their dependencies are identified
- [ ] Rollback strategy is documented

## Prompt Template
```
You are assisting with a refactor in the Test Engine repository.

**Phase 1: Context Gathering**
1. Read `docs/README.md` to understand the documentation hierarchy
2. Review `docs/architecture.md` to internalize system invariants
3. Examine the relevant specs: {spec_paths}
4. Check the task record: {task_path}
5. Review affected module READMEs: {module_paths}

**Phase 2: Impact Analysis**
1. Identify all modules touched by this refactor
2. List architectural invariants that must be preserved (cite `docs/architecture.md`)
3. Determine if any ADRs need updating or creation
4. Map dependencies using the build target inventory in root README
5. Identify potential breaking changes for downstream consumers

**Phase 3: Plan Development**
Produce a step-by-step implementation plan that includes:
1. Order of file modifications (respecting module dependencies)
2. Test strategy for each step (unit, integration, regression)
3. Documentation updates required (READMEs, specs, tasks)
4. Migration notes for API changes
5. Rollback checkpoints if the refactor must be abandoned

**Phase 4: Validation Strategy**
Define how you will verify:
1. All tests pass (`ctest --preset <preset>`)
2. Documentation validates (`python scripts/validate_docs.py`)
3. No architectural invariants violated
4. Performance characteristics maintained (if applicable)
5. Python bindings remain compatible (if applicable)

**Output Format:**
- Cite all file paths and line numbers for references
- Use markdown checkboxes for actionable items
- Group related changes by module
- Highlight breaking changes with ⚠️ markers
- Note any deferred work with links to follow-up tasks
```

## Common Refactoring Patterns

### Module Dependency Inversion (e.g., DC-001)
**Trigger:** Direct coupling between high-level orchestrator and concrete implementations

**Pattern:**
1. Define interface in lower-level module (e.g., `ISubsystemInterface` in Core)
2. Move concrete implementations to plugins
3. Introduce registry/factory for discovery
4. Update orchestrator to depend only on interface
5. Add build flags to make implementations optional

**Validation:**
- Build succeeds with subsets of implementations disabled
- Tests pass with mock implementations
- Documentation reflects new plugin contract

### Error Handling Migration (e.g., DC-004)
**Trigger:** Inconsistent error propagation (exceptions vs return codes)

**Pattern:**
1. Define `Result<T, Error>` and module-specific error codes
2. Migrate one function at a time, starting with leaf functions
3. Update call sites to check results explicitly
4. Add tests for error paths
5. Document migration status in module README

**Validation:**
- Static analysis confirms `[[nodiscard]]` usage
- Error paths have test coverage
- API documentation includes error cases

### Resource Lifetime Refactoring (e.g., AI-001)
**Trigger:** Opaque handles, stale reference bugs, unclear ownership

**Pattern:**
1. Introduce `ResourcePool<T, Tag>` with generational handles
2. Migrate caches to use typed handles
3. Add validation in debug builds
4. Update documentation with ownership patterns
5. Add lifetime violation tests

**Validation:**
- Debug builds catch stale handle access
- Pool iteration maintains deterministic order
- Handle recycling works correctly under stress

## Example Refactor Checklist
```
## Refactor: Introduce Async Asset Streaming (AI-002)

### Context
- [ ] Read `docs/design/async_streaming.md`
- [ ] Review `docs/modules/assets/README.md`
- [ ] Check dependencies: AI-001, DC-001

### Impact Analysis
- [ ] Modules affected: Assets, IO, Runtime, Core
- [ ] Invariants preserved: generational handles, deterministic loading
- [ ] Breaking changes: None (additive API)
- [ ] Dependencies: Requires thread pool in Core

### Implementation Plan
1. [ ] Add `IoThreadPool` to `engine/core/threading/`
2. [ ] Implement `AssetLoadRequest` and `AssetLoadFuture` primitives
3. [ ] Extend each cache with `schedule_async()` method
4. [ ] Wire thread pool into `RuntimeHost::initialize()`
5. [ ] Add integration tests with deterministic fixtures
6. [ ] Document usage in Assets README

### Validation
- [ ] Unit tests pass for request lifecycle
- [ ] Integration test demonstrates async load during tick
- [ ] Telemetry reports accurate timing
- [ ] Documentation validated with scripts

### Rollback Strategy
If integration tests fail:
1. Revert RuntimeHost changes
2. Keep new primitives as dead code
3. File issue linking this task record
```

## Anti-Patterns to Avoid

### ❌ Don't: Modify public APIs without migration guide
Instead: Document old and new usage, provide deprecation timeline

### ❌ Don't: Change behavior silently
Instead: Add new functions/flags, mark old ones deprecated

### ❌ Don't: Mix refactoring with feature work
Instead: Separate into distinct commits/PRs

### ❌ Don't: Skip test updates
Instead: Update tests alongside implementation

### ❌ Don't: Leave TODO comments without task records
Instead: Create task file in `docs/tasks/` and link it

## Post-Refactor Checklist

- [ ] All checkboxes in implementation plan marked complete
- [ ] Tests added and passing across all presets
- [ ] Documentation updates merged (READMEs, specs, tasks)
- [ ] Roadmap items updated with completion status
- [ ] Performance benchmarks recorded if applicable
- [ ] Migration guide published if API changed
- [ ] Follow-up tasks created for deferred work

## Integration with Architecture Improvement Plan

When implementing roadmap items:
1. Start with the canonical definition in `docs/ROADMAP.md#architecture-improvement-plan`
2. Check dependencies (other DC-*, AI-*, RT-* items)
3. Update the checklist in the roadmap as you complete tasks
4. Mirror completion status to module ROADMAPs
5. Update root README snapshot when major milestones close
