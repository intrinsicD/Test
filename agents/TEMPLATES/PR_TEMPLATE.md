# Pull Request Template

## Title
`<Type>(<Module>): <Short description>`

Examples:
- `feat(rendering): Add Vulkan frame graph MVP`
- `fix(geometry): Correct octree SAH split calculation`
- `refactor(math): Convert Matrix to column-major layout`
- `docs(animation): Add blend tree tutorial`

---

## Summary

<Provide a concise summary of what this PR accomplishes>

---

## Related Issues/Tasks

- Closes #<issue-number>
- Implements: `<TASK-ID>`
- Related ADR: `ADR-<id>`

---

## Changes

### Added
- <New feature or capability>

### Changed
- <Modified behavior or interface>

### Fixed
- <Bug fix description>

### Deprecated
- <Deprecated functionality>

### Removed
- <Removed code or functionality>

---

## Technical Details

### Implementation Approach
<High-level description of how the change was implemented>

### API Changes
<List any breaking changes or new public APIs>

**Breaking Changes:**
- [ ] Yes (requires migration)
- [ ] No

**New Public APIs:**
```cpp
namespace engine::<module> {
    // New interfaces
}
```

### Data Layout Changes
<Describe any changes to memory layout or data structures>

---

## Testing

### Test Coverage
- [ ] Unit tests added/updated
- [ ] Integration tests added/updated
- [ ] Property/fuzz tests added (if applicable)
- [ ] Coverage ≥ 85% on touched lines

### Test Commands
```bash
# Build
cmake --build --preset <preset>

# Run tests
ctest --preset <preset> --output-on-failure

# Run specific tests
./cmake-build-<preset>/path/to/test
```

### Test Results
<Paste test output or link to CI results>

---

## Performance

### Benchmarks Run
- [ ] Micro-benchmarks
- [ ] Macro-benchmarks
- [ ] Tracy profiling

### Performance Results

| Metric | Before | After | Δ% | Status |
|--------|--------|-------|-----|--------|
| <Operation> throughput | <X> ops/s | <Y> ops/s | +<Z>% | ✅ |
| Memory usage | <X> MB | <Y> MB | +<Z>% | ✅ |
| Frame time | <X> ms | <Y> ms | +<Z>% | ✅ |

**Regression Status:**
- [ ] No regression (≤ 2%)
- [ ] Regression detected (requires justification)

**Justification (if regression):**
<Explain why the regression is acceptable>

---

## Documentation

### Updated Documentation
- [ ] API documentation
- [ ] Module README
- [ ] Code examples
- [ ] Migration guide (if breaking changes)
- [ ] ADR created/updated
- [ ] Changelog updated

### Documentation Links
- `docs/<path>`
- `examples/<path>`

---

## CI/Build Status

### Platforms Tested
- [ ] Linux (Clang-22)
- [ ] Linux (GCC-12)
- [ ] Windows (MSVC 19.3x)

### Static Analysis
- [ ] No warnings (`-Werror`)
- [ ] `clang-tidy` clean
- [ ] Sanitizers green (ASan, UBSan, TSan)

### CI Results
<Link to CI build or paste status>

---

## Risks & Follow-ups

### Known Risks
- <Risk 1>
- <Risk 2>

### Follow-up Tasks
- [ ] Task 1: <description> → Issue #<number>
- [ ] Task 2: <description> → Issue #<number>

---

## Review Checklist

### Code Quality
- [ ] Follows `CODING_STYLE.md`
- [ ] No TODO/FIXME without tracking issues
- [ ] Error handling uses `Result<T, E>` or status codes
- [ ] Logging and tracing instrumented
- [ ] No allocations in hot paths

### Testing
- [ ] All new code paths tested
- [ ] Edge cases and negative tests included
- [ ] Tests are deterministic

### Documentation
- [ ] Public APIs documented
- [ ] Complex algorithms have rationale comments
- [ ] Examples compile and run

### Performance
- [ ] Benchmarks updated
- [ ] No performance regressions > 2%
- [ ] Tracy zones added for hot paths

---

## Screenshots/Demo (if applicable)

<Add screenshots, videos, or demo output if relevant>

---

## Migration Guide (if breaking changes)

### What Changed
<Describe the breaking change>

### Migration Steps
1. <Step 1>
2. <Step 2>
3. <Step 3>

### Example Code
**Before:**
```cpp
// Old usage
```

**After:**
```cpp
// New usage
```

---

## Reviewer Notes

<Any specific areas you want reviewers to focus on>

---

## Definition of Done

- [ ] All acceptance criteria met
- [ ] Tests pass on all platforms
- [ ] Performance verified
- [ ] Documentation complete
- [ ] Code reviewed and approved
- [ ] CI green

---

**Ready for Review**: Yes / No

**Assigned Reviewers**: @<username>, @<username>
# Issue Template

## Title
`<Feature/Bug>: <Short value statement>`

## Value
<Describe the metric improvement or user story>

## Scope
<List modules involved, e.g., engine::rendering, engine::geometry>

## Description
<Detailed description of the feature or bug>

## Acceptance Tests
<List specific GTest cases and demo steps>

**Unit Tests:**
- [ ] Test case 1
- [ ] Test case 2
- [ ] Negative/edge cases

**Integration Tests:**
- [ ] Demo scenario 1
- [ ] Demo scenario 2

## Benchmarks
<Describe micro/macro benchmarks with target thresholds>

**Performance Targets:**
- Micro-benchmark: <metric> ≥ <target>
- Macro-benchmark: <metric> ≥ <target>
- Regression tolerance: ≤ 2%

## Dependencies
<Link related issues, PRs, or ADRs>

- Depends on: #<issue>
- Related ADR: `docs/adr/ADR-<id>.md`
- Blocks: #<issue>

## Risks
<Top 3 risks>

1. **Risk 1**: <description> → Mitigation: <plan>
2. **Risk 2**: <description> → Mitigation: <plan>
3. **Risk 3**: <description> → Mitigation: <plan>

## Priority
<RICE score or priority level>

- **Reach**: <number of users affected>
- **Impact**: <high/medium/low>
- **Confidence**: <percentage>
- **Effort**: <person-days>

## Assigned To
<Module lead or role>

## Labels
<Add appropriate labels: bug, feature, enhancement, good first issue, etc.>

