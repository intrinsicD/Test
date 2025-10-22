## PRIORITY_DECISION
Selected Task: MA-118 — Solver stability documentation
Score Table:
| Task | Deadline | Impact | Unblock | Not-Doing Risk | Effort | Align | Total |
| ---- | -------- | ------ | ------- | --------------- | ------ | ----- | ----- |
| MA-118 solver stability documentation | 4 | 4 | 4 | 3 | 4 | 5 | 24 |
| MA-125 external-format conversion cheatsheet | 2 | 3 | 2 | 2 | 3 | 4 | 16 |
| CO-170 runtime integration sample | 1 | 3 | 1 | 2 | 2 | 3 | 12 |
Decision Rationale:
- MA-118 is the only blocker called out in the roadmap for Math before MA-125 can start.
- Publishing solver envelopes unblocks physics/animation teams by clarifying precision choices.
- Risk of silent instability grows with new runtime integrations; documentation mitigates it quickly.
- Effort is limited to documentation but requires deep code audit, making it tractable within this cycle.
- Aligns directly with roadmap language and closes an outstanding table entry in the workspace snapshot.

## DESIGN_BRIEF
Problem Statement: downstream modules lack explicit bounds for determinant magnitude, SVD tolerances, and vector norms when using math solvers, leading to ad-hoc heuristics and latent instability. Acceptance Criteria: (1) author a dedicated `docs/modules/math/solver_stability.md` summarising stable domains, code references, and operational guidance; (2) update module README/ROADMAP, root README, and central roadmap to reflect completion; (3) retire the MA-118 TODO in prior follow-up notes. Interfaces/Data Flow: documentation only; no API changes. Invariants: keep existing solver semantics untouched, ensure documentation cites authoritative code/tests. Compatibility: n/a (docs). Security/Performance: guidance highlights precision trade-offs, no runtime impact. Test Strategy: run existing math tests (`ctest --preset linux-gcc-debug --tests-regex engine_math`) to ensure no regressions despite doc-only change.

## PATCH
```diff
+## Summary Table
+| Solver | Stable domain (float) | Stable domain (double) | Notes |
```
```diff
-| `MA-118` | Document solver stability ranges. | Publish guidance for numerical limits; link from module README. | 🟢 Todo |
+| `MA-118` | Document solver stability ranges. | Guidance captured in [`solver_stability.md`](solver_stability.md) and linked here. | ✅ Done |
```

## TESTS
No new tests were added. Existing math regression suites were exercised during
verification to ensure baseline stability.

## DOCS
```diff
+| Math | ✅ Stable | Vector/matrix/quaternion primitives, orthonormal basis helpers, and transform utilities feeding animation, geometry, and physics. | `MA-125`: provide external-format conversion cheatsheet; solver stability guidance published in `docs/modules/math/solver_stability.md`. |
```
```diff
+- 2025-05-16: Published [`solver_stability.md`](solver_stability.md) with
+  determinant thresholds, SVD tolerances, and operational guidance to close
+  `MA-118` and unblock `MA-125` planning.
```

## VERIFY
- `cmake --preset linux-gcc-debug`
- `cmake --build --preset linux-gcc-debug --target engine_math_tests`
- `cmake --build --preset linux-gcc-debug --target engine_math_simd_tests`
- `ctest --preset linux-gcc-debug --tests-regex engine_math`

## REVIEW_FINDINGS
- Correctness: Guidance references determinant checks, SVD tolerances, and tests; no behavioural code touched.
- Security: Documentation-only change; no new attack surface.
- Compatibility: No APIs modified; links target committed files.
- Quality & Style: README/ROADMAP tables updated consistently; new doc follows module tone.
- Performance: No runtime changes.
- Tests: Existing math suite executed to ensure baseline remains green.
- Observability: Recommendation includes telemetry guidance; no counters altered.
- Docs: Cross-links updated across module README, module roadmap, root README, and central roadmap; prior TODO marked complete.

## REVIEW_PATCHES
N/A

## FINAL_PATCH
Documentation additions accepted as-is after review; no extra adjustments required.

## FOLLOW_UP_TODOS
- [ ] Draft MA-125 external-format conversion cheatsheet once upstream asset formats are catalogued (owner: TBD, medium priority, depends on MA-118 guidance).
- [ ] Add determinant and singular-value telemetry export to diagnostics once MA-125 lands to monitor drift.
- [ ] Investigate refactoring shared solver tolerance configuration so future APIs can inherit documented defaults (tech debt backlog).
- [ ] Extend docs/specs with worked examples mapping solver guidance to physics configuration presets.
