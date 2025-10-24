# ADR-<id>: <Title>

**Status**: Proposed | Accepted | Deprecated | Superseded by ADR-<id>

**Date**: YYYY-MM-DD

**Authors**: <Names or roles>

---

## Context

<What is the background and problem that needs to be addressed?>

**Constraints:**
- Performance budget: <specific limits>
- Platform requirements: <platforms>
- Compatibility: <backward compatibility needs>
- Technical debt: <existing constraints>

**Related Documents:**
- Issue: #<number>
- Epic/Roadmap: <link>
- Related ADRs: ADR-<id>, ADR-<id>

---

## Decision

<What is the decision and why was it made?>

**Chosen Approach:**
<Detailed description of the solution>

**Rationale:**
- **Pro 1**: <benefit>
- **Pro 2**: <benefit>
- **Con 1**: <tradeoff>
- **Con 2**: <tradeoff>

**Alternatives Considered:**
1. **Alternative 1**: <description> → Rejected because: <reason>
2. **Alternative 2**: <description> → Rejected because: <reason>

---

## Consequences

### Positive
- <Positive consequence 1>
- <Positive consequence 2>

### Negative
- <Negative consequence 1>
- <Mitigation plan>

### Neutral
- <Neutral change that teams need to be aware of>

---

## Implementation Details

### Interfaces
<Public headers, APIs, and data structures>

```cpp
// Example interface
namespace engine::<module> {
    class <ClassName> {
        // Public API
    };
}
```

### Data Layout
<Memory layout, SoA vs AoS, GPU considerations>

### Cross-Module Impact
- **Module 1**: <impact and required changes>
- **Module 2**: <impact and required changes>

---

## Migration Plan

### Breaking Changes
- <Breaking change 1>
- <Breaking change 2>

### Migration Steps
1. <Step 1>
2. <Step 2>
3. <Step 3>

### Compatibility Strategy
- Deprecation timeline: <dates>
- Feature flag: `ENGINE_<FEATURE>_<VERSION>`
- Fallback behavior: <description>

---

## Testing Strategy

### Test Coverage
- **Unit tests**: <what needs to be tested>
- **Integration tests**: <scenarios>
- **Performance tests**: <benchmarks>

### Invariants to Maintain
- <Invariant 1>
- <Invariant 2>

### Acceptance Criteria
- [ ] All tests pass on CI (Clang-22, MSVC)
- [ ] Performance regression ≤ 2%
- [ ] Documentation updated
- [ ] Migration guide published

---

## Hard Rules

<Non-negotiable constraints that must be followed>

- **Rule 1**: <e.g., Stable ABI for plugins>
- **Rule 2**: <e.g., SoA for large arrays>
- **Rule 3**: <e.g., No allocations in frame hot paths>

---

## References

- **Research papers**: <links>
- **External libraries**: <links to docs>
- **Benchmarks**: <links to data>
- **Prior art**: <similar implementations>

---

## Review History

| Date | Reviewer | Decision | Comments |
|------|----------|----------|----------|
| YYYY-MM-DD | <Name> | Approved | <notes> |
| YYYY-MM-DD | <Name> | Requested changes | <notes> |

