# Task Card Template

## Task ID
`<MODULE>-<NUMBER>` (e.g., RE-541, GE-221)

## Title
<Concise title describing the task>

## Type
- [ ] Feature
- [ ] Bug Fix
- [ ] Refactor
- [ ] Documentation
- [ ] Research
- [ ] Performance Optimization

## Priority
- [ ] Critical (P0)
- [ ] High (P1)
- [ ] Medium (P2)
- [ ] Low (P3)

## Estimated Effort
<Time estimate: hours or days>

---

## Description

### Problem Statement
<What problem does this solve?>

### Proposed Solution
<High-level approach>

### Success Criteria
<How do we know this is complete?>

---

## Technical Details

### Scope
**Modules Affected:**
- `engine::<module>`

**Files to Modify:**
- `<path/to/file.hpp>`
- `<path/to/file.cpp>`
- `<path/to/test.cpp>`

**New Files:**
- `<path/to/new_file.hpp>`

### Dependencies
**Depends On:**
- Task: `<TASK-ID>`
- ADR: `ADR-<id>`
- Library: `<name> v<version>`

**Blocks:**
- Task: `<TASK-ID>`

### Related Work
- Issue: #<number>
- PR: #<number>
- Epic: <roadmap item>

---

## Acceptance Criteria

### Functional Requirements
- [ ] Requirement 1
- [ ] Requirement 2
- [ ] Requirement 3

### Non-Functional Requirements
- [ ] Performance: <metric> ≥ <target>
- [ ] Memory: Usage ≤ <limit>
- [ ] Latency: Response time ≤ <threshold>

### Testing Requirements
- [ ] Unit tests cover all new code paths
- [ ] Integration tests validate end-to-end behavior
- [ ] Benchmarks show ≤ 2% regression
- [ ] Coverage ≥ 85% on touched lines

### Documentation Requirements
- [ ] API docs updated
- [ ] Example code added
- [ ] Migration guide (if breaking changes)
- [ ] ADR created/updated

---

## Test Plan

### Unit Tests
```cpp
TEST(<Module>Test, <TestName>) {
    // Test setup
    // Assertions
}
```

### Integration Tests
<Describe scenarios>

### Performance Tests
<Benchmark parameters and expected results>

---

## Implementation Notes

### Design Considerations
- <Key design decision 1>
- <Key design decision 2>

### Risks & Mitigations
| Risk | Likelihood | Impact | Mitigation |
|------|------------|--------|------------|
| <Risk 1> | High/Med/Low | High/Med/Low | <Plan> |
| <Risk 2> | High/Med/Low | High/Med/Low | <Plan> |

### Alternative Approaches
1. **Alternative 1**: <description> → <why not chosen>
2. **Alternative 2**: <description> → <why not chosen>

---

## Deliverables

- [ ] Code implementation
- [ ] Unit tests
- [ ] Integration tests
- [ ] Benchmarks
- [ ] API documentation
- [ ] Example code
- [ ] PR opened and linked
- [ ] All CI checks passing

---

## Definition of Done

- [ ] Builds cleanly on CI (Clang-22, MSVC)
- [ ] All tests pass (unit, integration, sanitizers)
- [ ] Performance regression ≤ 2%
- [ ] Code coverage ≥ 85% on touched lines
- [ ] Documentation updated and reviewed
- [ ] Code review approved by Tech Lead
- [ ] PR merged to main

---

## Assigned To
**Role**: <Tech Lead / Engineer / etc.>
**Name**: <@username>

## Estimated Timeline
**Start Date**: YYYY-MM-DD
**Target Completion**: YYYY-MM-DD
**Actual Completion**: YYYY-MM-DD

---

## Notes
<Any additional context, links, or discussion points>

