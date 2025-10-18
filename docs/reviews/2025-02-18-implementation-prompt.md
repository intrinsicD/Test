# Review: Implementation prompt migration (2025-02-18)

Following the checklist in [`docs/prompts/review-checklist.md`](../prompts/review-checklist.md).

## Summary
The existing implementation prompt outlines a dual-phase implementer/reviewer workflow but lacks the structural cues and
repository alignment present in our other prompts.

## Architectural Impact
- Documentation-only scope; no code or build configuration touched.
- Ensures prompt catalogue stays consistent with Test Engine guardrails and documentation hierarchy.

## Findings

### Critical Issues 🔴
None.

### Warnings ⚠️
1. **Missing prompt framing and prerequisites.**
   - The prompt begins directly with the system role without a title, "When to Use" guidance, or prerequisite checklist, unlike
     `docs/prompts/architecture-audit.md` and peers.
   - **Recommendation:** Refactor into the standard prompt layout so contributors know when and how to apply it.
2. **Repository standards drift.**
   - The "PROJECT_STANDARDS" section references JavaScript, Go, and generic API/DB policies that do not exist in this
     repository, while omitting C++ requirements from `CODING_STYLE.md`.
   - **Recommendation:** Replace the mixed-language list with Test Engine-specific C++/Python expectations.

### Suggestions 💡
1. **Normalize output schema formatting.**
   - Several numbered headings (e.g., `3.## PATCH`) omit a separating space, which breaks Markdown rendering and makes the
     schema harder to scan.
   - **Follow-up:** Adjust numbering to `N. ## Heading` during the refactor.

## Documentation Status
- [ ] `docs/prompts/architecture-audit.md`
- [ ] `docs/prompts/refactor-playbook.md`
- [ ] `docs/prompts/review-checklist.md`
- [ ] `docs/README.md`

## Test Coverage
Not applicable (documentation review).

## Follow-Up Work
- [ ] Publish the refactored prompt under `docs/prompts/` and update navigation to point to the new location.

## Verdict
- [x] 🔄 Request Changes (address warnings before adopting the prompt)
