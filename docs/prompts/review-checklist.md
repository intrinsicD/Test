# Code Review Checklist Prompt

Use this prompt to evaluate pull requests consistently in the Test Engine repository.

## When to Use
- All pull requests before merging
- Post-merge audits for architectural drift
- Pre-release validation passes

## Prompt Template
```
You are conducting a code review for the Test Engine repository.

**Phase 1: Documentation Context**
1. Read `docs/agents.md` to understand review priorities and guardrails
2. Review `docs/architecture.md` to internalize system invariants
3. Examine `docs/conventions.md` for coding/testing standards
4. Check relevant specs in `docs/specs/` if architectural changes are present

**Phase 2: Change Analysis**
1. Identify all modified files and group by module
2. For each affected module:
   - Open its README in `docs/modules/<module>/README.md`
   - Check its roadmap in `docs/modules/<module>/ROADMAP.md`
   - Review dependencies from the build target inventory
3. Determine if changes relate to roadmap items (DC-*, AI-*, RT-*)
4. Check if any specs/tasks mentioned in commits are updated

**Phase 3: Architectural Review**
Verify that the changes preserve these invariants:

**Resource Management (AI-001):**
- [ ] New resources use `ResourcePool` with generational handles
- [ ] Handle validation occurs before dereference
- [ ] Ownership patterns documented

**Error Handling (DC-004):**
- [ ] Public APIs return `Result<T, Error>` for recoverable failures
- [ ] Error codes are module-specific enums
- [ ] Functions marked `[[nodiscard]]` appropriately

**Module Boundaries:**
- [ ] No circular dependencies introduced
- [ ] Public headers expose minimal surface
- [ ] Implementation details hidden in `src/` or unnamed namespaces

**Determinism (Frame Graph, Scheduler):**
- [ ] Frame-graph compilation remains deterministic
- [ ] Resource transitions explicit and validated
- [ ] Queue affinity properly declared

**Testing Discipline:**
- [ ] New behavior covered by unit tests
- [ ] Bugfixes include regression tests
- [ ] Integration tests updated for cross-module changes

**Phase 4: Documentation Verification**
Check that documentation is updated:

- [ ] Module READMEs reflect current behavior
- [ ] Roadmap checklists updated if completing tasks
- [ ] Root README snapshot aligned with major changes
- [ ] ADRs created/updated for architectural decisions
- [ ] Task records reflect completion status
- [ ] Migration guides provided for breaking changes

**Phase 5: Code Quality Assessment**

**Style & Conventions:**
- [ ] C++20 idioms used correctly
- [ ] Naming follows `CapitalCase` for types, `snake_case` for functions/variables
- [ ] Comments explain *why* and *how*, not just *what*
- [ ] No unnecessary includes or transitive dependencies

**Performance:**
- [ ] No allocations in hot loops (geometry, physics)
- [ ] Cache-friendly data layouts maintained
- [ ] SIMD opportunities noted if relevant

**Safety:**
- [ ] Bounds checking for array/buffer access
- [ ] Null pointer checks before dereference
- [ ] Resource cleanup in error paths
- [ ] Thread safety annotations if concurrent

**Phase 6: Testing Validation**
- [ ] `ctest --preset <preset>` passes for affected modules
- [ ] `pytest python/tests scripts/tests` passes if Python changed
- [ ] `python scripts/validate_docs.py` succeeds
- [ ] Integration tests demonstrate end-to-end behavior

**Output Format:**
Provide a structured review with these sections:

## Summary
[One paragraph overview of the change and its purpose]

## Architectural Impact
[List affected invariants, roadmap items, and module dependencies]

## Findings

### Critical Issues 🔴
[Blocking problems that must be fixed before merge]
- Issue description
- Affected files: `path/to/file.cpp:line`
- Recommendation: [specific fix]

### Warnings ⚠️
[Non-blocking concerns that should be addressed]
- Issue description
- Affected files: `path/to/file.hpp:line`
- Recommendation: [specific fix or follow-up task]

### Suggestions 💡
[Optional improvements for future consideration]
- Suggestion description
- Rationale: [why this would be better]
- Follow-up task: [link to task record if created]

## Documentation Status
[List all documentation files that were/should be updated]
- [ ] `docs/modules/<module>/README.md`
- [ ] `docs/modules/<module>/ROADMAP.md`
- [ ] `docs/ROADMAP.md` (if roadmap item completed)
- [ ] `README.md` (if snapshot needs updating)
- [ ] Relevant specs in `docs/specs/`

## Test Coverage
[Summarize test additions/changes]
- Unit tests: [count] added, [count] modified
- Integration tests: [description]
- Regression coverage: [specific bugs/behaviors covered]

## Follow-Up Work
[List any deferred tasks or future improvements]
- [ ] Task description → `docs/tasks/T-XXXX-short-name.md`

## Verdict
- [ ] ✅ Approve (ready to merge)
- [ ] 🔄 Request Changes (critical issues must be addressed)
- [ ] 💬 Comment (feedback provided, no blocking issues)
```

## Review Gate Criteria

A PR should not be merged unless:

### Correctness Gates
- [ ] Builds successfully on all configured presets
- [ ] All tests pass (`ctest`, `pytest`)
- [ ] No compiler warnings introduced
- [ ] No memory leaks (valgrind/sanitizers if available)

### Architectural Gates
- [ ] Invariants listed in `docs/architecture.md` preserved
- [ ] Module boundaries respected (no new circular dependencies)
- [ ] Resource lifetimes follow `AI-001` patterns
- [ ] Error handling follows `DC-004` patterns

### Documentation Gates
- [ ] All affected module READMEs updated
- [ ] Roadmap items marked complete if applicable
- [ ] ADRs created/updated for architectural decisions
- [ ] Migration guides provided for breaking changes
- [ ] `scripts/validate_docs.py` passes

### Testing Gates
- [ ] New features have unit tests
- [ ] Bugfixes have regression tests
- [ ] Integration tests cover cross-module interactions
- [ ] Performance-critical paths have benchmarks

## Common Issues and Resolutions

### Issue: Missing Roadmap Update
**Symptom:** PR implements `DC-001` but doesn't update `docs/ROADMAP.md`

**Resolution:**
1. Check off completed tasks in the roadmap checklist
2. Update priority horizon if milestone shifts
3. Mirror changes to affected module ROADMAPs
4. Update root README snapshot if major item closes

### Issue: Stale Documentation
**Symptom:** Module README describes old behavior

**Resolution:**
1. Update "Current State" section with new behavior
2. Add usage examples showing new API
3. Note any deprecations or migrations needed
4. Run `scripts/validate_docs.py` to catch broken links

### Issue: Insufficient Test Coverage
**Symptom:** New function has no tests

**Resolution:**
1. Add unit tests in `engine/<module>/tests/`
2. Wire test target into CMakeLists.txt
3. Add integration test if cross-module
4. Document test strategy in PR description

### Issue: Breaking Change Without Migration Guide
**Symptom:** Public API changed, no deprecation warning

**Resolution:**
1. Restore old API as deprecated
2. Add new API alongside
3. Create migration guide in module README
4. Set deprecation timeline (e.g., 2 releases)
5. Update Python bindings if applicable

### Issue: Architectural Invariant Violation
**Symptom:** Direct dependency from Runtime to concrete implementations

**Resolution:**
1. Reference relevant ADR (e.g., `ADR-0003`)
2. Explain which invariant is violated
3. Suggest dependency inversion pattern
4. Link to similar implementations in codebase

## Review Examples

### Example 1: Architectural Refactoring
```markdown
## Summary
Implements `DC-001` by introducing subsystem plugin interfaces. Runtime now
discovers subsystems through `SubsystemRegistry` instead of hardcoding
dependencies.

## Architectural Impact
- Breaks circular dependency: Runtime no longer links all modules
- Enables optional subsystems via `ENGINE_ENABLE_<MODULE>` flags
- Aligns with plugin architecture established in `DC-001`

## Findings

### Critical Issues 🔴
None

### Warnings ⚠️
1. `SubsystemRegistry::register_descriptor` is not thread-safe
   - Affected files: `engine/runtime/src/subsystem_registry.cpp:45`
   - Recommendation: Add mutex or document as main-thread-only

### Suggestions 💡
1. Consider adding subsystem dependency validation
   - Rationale: Catch cycles at registration time
   - Follow-up: Create `T-0120-subsystem-dependency-validation.md`

## Documentation Status
- [x] `docs/modules/runtime/README.md` - plugin contract added
- [x] `docs/modules/core/README.md` - ISubsystemInterface documented
- [x] `docs/ROADMAP.md` - DC-001 checklist completed
- [x] `README.md` - snapshot updated with new configuration

## Test Coverage
- Unit tests: 5 added (`test_subsystem_registry.cpp`)
- Integration tests: Runtime initializes with subset of modules
- Regression coverage: Repeated init/shutdown cycles

## Follow-Up Work
- [ ] Add thread-safety analysis → `docs/tasks/T-0120-...`
- [ ] Document performance implications → module README

## Verdict
- [x] ✅ Approve with follow-up tasks created
```

### Example 2: Bug Fix with Insufficient Coverage
```markdown
## Summary
Fixes crash when releasing invalid resource handles in debug builds.

## Architectural Impact
Minimal - internal validation logic only.

## Findings

### Critical Issues 🔴
1. Missing regression test for the reported crash
   - Affected files: `engine/core/tests/` (missing test)
   - Recommendation: Add test that attempts double-release

### Warnings ⚠️
None

## Documentation Status
No documentation changes required (internal fix).

## Test Coverage
- Unit tests: 0 added ⚠️
- Regression coverage: None ⚠️

## Follow-Up Work
- [ ] Add regression test before merge

## Verdict
- [ ] 🔄 Request Changes - test coverage required
```

## Integration with Development Workflow

### Before Starting Review
1. Check if PR references a task record (`docs/tasks/T-XXXX-...`)
2. Review the task's acceptance criteria
3. Verify the change maps to a roadmap item

### During Review
1. Use this prompt template
2. Open relevant documentation in parallel
3. Run tests locally if uncertain
4. Check diffs against architectural patterns

### After Review
1. Update task record with review feedback
2. Create follow-up tasks for deferred work
3. Notify module owners if architectural concerns arise
