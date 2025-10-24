# CONTEXT PACK

**Scope**: `<module/paths>`

**Objective**: <Task summary in 1-2 sentences>

**Token Budget**: <Approximate tokens available, e.g., ~8000>

---

## Repo Manifest (Relevant Files)

<List only the files needed for this task>

- `<path/to/file.hpp>`: <Why relevant - e.g., "Public API for the component">
- `<path/to/file.cpp>`: <Why relevant - e.g., "Implementation details">
- `<path/to/test.cpp>`: <Why relevant - e.g., "Existing test coverage">
- `<path/to/benchmark.cpp>`: <Why relevant - e.g., "Performance baseline">

---

## File Excerpts

<Provide trimmed excerpts of the most critical sections>

### `<path/to/file.hpp>`

```cpp
// Critical interface or declaration
namespace engine::<module> {
    // ...existing code...
    
    class ImportantClass {
        // Key methods or data
    };
}
```

### `<path/to/file.cpp>`

```cpp
// Key implementation details
void ImportantClass::key_method() {
    // ...existing implementation...
}
```

### `<path/to/test.cpp>`

```cpp
// Existing test patterns
TEST(ModuleTest, ExistingBehavior) {
    // Test setup and assertions
}
```

---

## Build & Test Commands

### Configure
```bash
cmake --preset <preset-name>
```

### Build
```bash
cmake --build --preset <preset-name>
```

### Run Tests
```bash
ctest --preset <preset-name> --output-on-failure
```

### Run Specific Test
```bash
./cmake-build-<preset>/engine/<module>/tests/<test-name>
```

### Run Benchmarks
```bash
./cmake-build-<preset>/engine/<module>/bench/<benchmark-name>
```

---

## Open Threads

**Related Issues:**
- Issue #<number>: <brief description>
- Issue #<number>: <brief description>

**Related ADRs:**
- ADR-<id>: <title> - <relevance>

**Related PRs:**
- PR #<number>: <brief description> - <status>

**Open Questions:**
- <Question 1>
- <Question 2>

---

## Acceptance Criteria

### Tests
- [ ] `<TestName1>`: <what it validates>
- [ ] `<TestName2>`: <what it validates>
- [ ] Edge cases covered: <list>

### Benchmarks
- [ ] `<BenchmarkName1>`: Target ≥ <metric>
- [ ] Regression tolerance: ≤ 2% vs. baseline
- [ ] Memory usage: ≤ <threshold>

### Documentation
- [ ] API documentation updated
- [ ] Example code added to `docs/examples/<module>/`
- [ ] Migration notes if breaking changes

### Code Quality
- [ ] Builds cleanly on CI (Clang-22, MSVC)
- [ ] Sanitizers green (ASan, UBSan)
- [ ] Coverage ≥ 85% on touched lines
- [ ] Follows `CODING_STYLE.md`

---

## Performance Baselines

<Current performance numbers to compare against>

| Metric | Current | Target | Tolerance |
|--------|---------|--------|-----------|
| <Operation> throughput | <X> ops/sec | ≥ <Y> ops/sec | ±2% |
| Memory usage | <X> MB | ≤ <Y> MB | +5% |
| Frame time impact | <X> ms | ≤ <Y> ms | +0.5ms |

---

## Dependencies & Environment

**Required Libraries:**
- <Library 1>: version <X.Y.Z>
- <Library 2>: version <X.Y.Z>

**Platform-Specific Notes:**
- Linux: <notes>
- Windows: <notes>

**Build Configuration:**
- CMake preset: `<preset-name>`
- Compiler: Clang-22 / MSVC 19.3x
- Build type: Debug / Release

---

## Session Constraints

**Time Budget**: <Expected effort, e.g., 2-4 hours>

**Scope Limits**:
- Only modify files listed in the manifest
- Do not introduce new dependencies without approval
- Keep diffs < 400 LOC

**Handoff Plan**:
- **Next Role**: <Reviewer / Performance Engineer / etc.>
- **Artifacts to Deliver**: <PR / Benchmark results / Documentation>

