# 2026-01-08 Application Readiness Assessment

## Scope & Method

This review evaluates how close the Test Engine is to powering a research or prototyping application. Inputs included the module
health snapshot in [`README.md`](../../README.md), the active initiative breakdown in [`docs/ROADMAP.md`](../ROADMAP.md), and the
P0 task briefs for `AI-004` and its constituent workstreams (`RE-610`, `RT-320`, `T-0119`, `T-0120`, `TL-210`, `CC-310`). The
assessment follows the agent workflow guardrails in [`../../AGENTS.md`](../../AGENTS.md): inspect authoritative docs first, score readiness along product and technical
dimensions, and surface the minimum set of actions required to ship an end-to-end prototype experience.

## Readiness Summary

| Dimension | Score (1–5) | Notes |
| --- | --- | --- |
| Product Enablement | 3 | All core modules report **Stable** status with clear next tasks, but the prototyping workflow in `AI-004` remains in planning with unresolved risks around configuration schema enforcement, dataset licensing, and benchmark infrastructure. |
| Technical Integration | 2 | Rendering baseline preset is complete, yet critical GPU resource and command encoding layers remain partially stubbed; runtime harness and sandbox automation still list open acceptance criteria. |
| Documentation & Onboarding | 4 | Roadmap, tasks, and README stay synchronized, and harness CLI scaffolding exists, but a full prototyping playbook and benchmark quickstart remain unchecked deliverables. |

**Overall Readiness**: **2.75 / 5** — The engine’s subsystems are production-quality, but application-facing tooling is still in
Phase 1 of `AI-004`, leaving interactive prototypes dependent on mock GPU backends and manual coordination.

## Strengths Already in Place

1. **Subsystem maturity** – Every engine module is marked “✅ Stable” with detailed capabilities captured in the workspace snapshot,
   providing a solid base for higher-level tooling.
2. **Rendering preset completed** – `RE-610` shipped a research rendering baseline with forward/deferred pipelines, debug overlays,
   telemetry, and an example application that demonstrates frame-graph usage.
3. **Harness scaffolding** – The runtime task card and progress notes confirm a Python-backed harness CLI, schema validation flags,
   and sample manifests already exist, indicating cross-module alignment is underway.

## Critical Gaps to Application Use

1. **GPU resource creation is unfinished** – `T-0120` still lacks real resource allocation across backends, leaving the engine
   dependent on recording providers and preventing real draw submissions on anything but mock paths.
2. **Command encoding is incomplete** – `T-0119` retains outstanding backend bindings; without Vulkan/DX12/Metal encoders the
   frame graph cannot produce native GPU workloads required for production tests.
3. **Runtime harness functionality is partial** – `RT-320` keeps all functional acceptance criteria unchecked; interactive and
   headless flows remain to be implemented along with telemetry regression coverage.
4. **AI-004 milestones still planning** – Roadmap entries for `AI-004` list every subtask as 🟡 Planning; Phase 1 checkpoints and
   risk mitigations must complete before the initiative can graduate to execution.
5. **Risk register deadlines lapsed** – Kickoff risks (schema alignment, dataset licensing, benchmark hardware) have due dates in
   late 2025 with mitigation decks still pending, indicating coordination debt before a prototype release can be endorsed.

## Recommendations

1. **Prioritise GPU resource + command encoding** – Close `T-0120` and `T-0119` before scheduling external demos; without real
   resource creation and draw submission the rendering baseline cannot leave headless/mock pipelines.
2. **Graduate runtime harness to feature-complete** – Finish `RT-320` acceptance criteria, expand tests, and document CLI/UI flows
   so geometry case studies can run without manual scripting.
3. **Publish AI-004 kickoff packet** – Execute `DC-041` and refresh the risk register so owners, due dates, and mitigations reflect
   current status.
4. **Author the prototyping playbook** – Document the end-to-end workflow (schema → dataset → harness → sandbox → benchmarks) and
   link it through `docs/NAVIGATION.md` to unblock new contributors.
5. **Stage comparative benchmarking** – Advance `CC-310`/`CC-311` to integrate telemetry plots and CI gating, ensuring prototype
   results remain reproducible once the harness is feature-complete.

## Readiness Outlook

Assuming the P0 blockers above close in the next sprint and documentation catches up, the engine could support internally scoped
application prototypes within one to two milestones. External-facing or production demos depend on completing GPU backend support
beyond OpenGL, stabilising dataset licensing, and codifying benchmark governance.
