# Hybrid Agentic Workflow - Visual Guide

```
┌─────────────────────────────────────────────────────────────────────────┐
│                         HYBRID WORKFLOW ENTRY POINTS                    │
├─────────────────────────────────────────────────────────────────────────┤
│                                                                         │
│  👤 HUMAN CONTRIBUTORS          🤖 AI AGENTS                            │
│  ↓                              ↓                                       │
│  README.md ─────────────────→   agents/AGENTS-INDEX.md                 │
│  │                              │                                       │
│  ├→ docs/HYBRID_WORKFLOW.md ←──┘                                       │
│  │  (Complete Guide)                                                    │
│  │                                                                      │
│  ├→ docs/NAVIGATION.md                                                 │
│  │  (Documentation Map)                                                │
│  │                                                                      │
│  └→ CODING_STYLE.md                                                    │
│     (Standards)                                                         │
└─────────────────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────────────────┐
│                    WORKFLOW PHASES (Old + New Combined)                 │
└─────────────────────────────────────────────────────────────────────────┘

PHASE 1: DISCOVERY & PLANNING
┌────────────────────────────────────────────────────────────────┐
│ 🎯 Product Manager (10)                                        │
│    ├─ Reads: docs/ROADMAP.md (OLD: strategic alignment)       │
│    ├─ Creates: docs/archive/backlog/legacy/tasks/MODULE-NNN.md (OLD: task location)  │
│    ├─ Uses: Module-based IDs (OLD: RE-541, GE-221)           │
│    └─ Template: TEMPLATES/TASK_CARD.md (NEW: formalized)      │
│                                                                │
│ 🎭 Agent Orchestrator (11)                                     │
│    ├─ Reviews: README.md module health (OLD: status check)    │
│    ├─ Builds: Context Pack (NEW: formalized handoff)          │
│    ├─ Routes: To specialist role (NEW: explicit coordination) │
│    └─ Template: TEMPLATES/CONTEXT_PACK.md (NEW)               │
└────────────────────────────────────────────────────────────────┘

PHASE 2: DESIGN & ARCHITECTURE (When Needed)
┌────────────────────────────────────────────────────────────────┐
│ 🏛️ Chief Architect (20)                                        │
│    ├─ Reviews: docs/ARCHITECTURE.md (OLD: alignment)          │
│    ├─ Creates: ADR in docs/specs/ (NEW: formalized)           │
│    ├─ Template: TEMPLATES/ADR_TEMPLATE.md (NEW)               │
│    └─ Updates: Module interfaces (OLD: header-first)          │
│                                                                │
│ 🔬 Research Scientist (13)                                     │
│    ├─ Evaluates: Algorithms and techniques                    │
│    ├─ Creates: Prototypes and benchmarks (OLD: proof)         │
│    └─ Writes: RFPs for major decisions (NEW: formalized)      │
└────────────────────────────────────────────────────────────────┘

PHASE 3: IMPLEMENTATION
┌────────────────────────────────────────────────────────────────┐
│ 📚 Required Reading (OLD: practical guidance)                  │
│    1. README.md - Module health, build workflow               │
│    2. docs/NAVIGATION.md - Documentation structure             │
│    3. CODING_STYLE.md - C++20/Python standards                │
│    4. docs/ROADMAP.md - Current priorities                     │
│    5. agents/00-COMMON-GUARDRAILS.md - Universal rules        │
│                                                                │
│ 🏗️ Standard Build Workflow (OLD: always accessible)           │
│    cmake --preset <debug|release|sanitize>                     │
│    cmake --build --preset <debug|release|sanitize>             │
│    ctest --preset <debug|release|sanitize> --output-on-failure │
│    pytest python/tests scripts/tests                           │
│                                                                │
│ 💻 Module Engineers (NEW: specialized roles)                   │
│    ├─ Tech Lead (30) - Cross-module coordination              │
│    ├─ Rendering Engineer (40) - Graphics pipeline             │
│    ├─ Geometry/Math Engineer (50) - Spatial structures        │
│    └─ Physics Engineer (60) - Collision & dynamics            │
│                                                                │
│ 🛠️ Infrastructure (NEW: specialized roles)                     │
│    ├─ Tools/Build/CI Engineer (70) - CMake, CI pipelines      │
│    └─ Follow: Module README in docs/modules/<name>/           │
└────────────────────────────────────────────────────────────────┘

PHASE 4: QUALITY ASSURANCE (Parallel Gates)
┌────────────────────────────────────────────────────────────────┐
│ 🧪 QA/Test Engineer (90) - NEW: specialized testing           │
│    ├─ Coverage: ≥ 85% on touched lines (OLD: metric)          │
│    ├─ Determinism: All tests reproducible (OLD: requirement)  │
│    └─ Integration: Cross-module tests (OLD: requirement)      │
│                                                                │
│ ⚡ Performance Engineer (80) - NEW: specialized perf           │
│    ├─ Benchmarks: No regressions > 5% (OLD: metric)           │
│    ├─ Telemetry: Integrated (OLD: requirement)                │
│    └─ Profiling: Tracy zones (OLD: requirement)               │
│                                                                │
│ 🔒 Security/Safety Gate (15) - NEW: dedicated security        │
│    ├─ Sanitizers: All green (OLD: requirement)                │
│    ├─ Dependencies: Audit updates (NEW: formalized)           │
│    └─ Safety: No UB (OLD: requirement)                        │
│                                                                │
│ 📖 Docs/DevRel (95) - NEW: documentation specialist           │
│    ├─ API Docs: Updated in same PR (OLD: requirement)         │
│    ├─ Examples: Compile and demonstrate (OLD: requirement)    │
│    └─ Module README: Updated if behavior changed (OLD: req)   │
└────────────────────────────────────────────────────────────────┘

PHASE 5: REVIEW & RELEASE
┌────────────────────────────────────────────────────────────────┐
│ ✅ Reviewer (99) - NEW: explicit review role                   │
│    ├─ Validates: Definition of Done (OLD: criteria)           │
│    ├─ Checks: All quality gates passed (NEW: formalized)      │
│    ├─ Reviews: Using PR template (NEW: consistency)           │
│    └─ Template: TEMPLATES/PR_TEMPLATE.md (NEW)                │
│                                                                │
│ 📦 Release Manager (98) - NEW: release coordination           │
│    ├─ Versioning: Semantic versioning (OLD: convention)       │
│    ├─ Changelog: Release notes (OLD: requirement)             │
│    └─ Artifacts: Build and publish (NEW: formalized)          │
└────────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────────────────┐
│                        TASK MANAGEMENT (HYBRID)                         │
└─────────────────────────────────────────────────────────────────────────┘

THREE-TIER STRUCTURE (Best of both workflows):

1. ACTIVE TASKS (OLD: clear structure)
   Location: docs/archive/backlog/legacy/tasks/
   Format:   <MODULE>-<NUMBER>.md (e.g., RE-541.md)
   Template: TEMPLATES/TASK_CARD.md (NEW: formalized)
   Tracking: README.md module health table (OLD: visibility)

2. STRATEGIC ROADMAP (OLD: authoritative backlog)
   Location: docs/ROADMAP.md
   Format:   Initiative IDs (DC-004, AI-002)
   Linkage:  Tasks reference parent initiatives (OLD: traceability)

3. COMMUNITY ISSUES (NEW: external engagement)
   Location: GitHub Issues
   Template: TEMPLATES/ISSUE_TEMPLATE.md (NEW: formalized)
   Labels:   bug, feature, good first issue (NEW: categorization)

MODULE PREFIX TABLE (OLD: maintained from original workflow):
┌──────┬────────────┬──────────────────┐
│ Code │ Module     │ Example          │
├──────┼────────────┼──────────────────┤
│ AN   │ Animation  │ AN-230           │
│ AS   │ Assets     │ AS-150           │
│ CO   │ Compute    │ CO-170           │
│ CR   │ Core       │ CR-135           │
│ GE   │ Geometry   │ GE-221           │
│ IO   │ IO         │ IO-240           │
│ MA   │ Math       │ MA-130           │
│ PH   │ Physics    │ PH-430           │
│ PL   │ Platform   │ PL-215           │
│ RE   │ Rendering  │ RE-541           │
│ RT   │ Runtime    │ RT-006           │
│ SC   │ Scene      │ SC-230           │
│ DC   │ Docs       │ DC-004           │
│ AI   │ Arch Init  │ AI-002           │
└──────┴────────────┴──────────────────┘

┌─────────────────────────────────────────────────────────────────────────┐
│                    DEFINITION OF DONE (UNIFIED)                         │
└─────────────────────────────────────────────────────────────────────────┘

Every PR must meet ALL criteria (OLD metrics + NEW enforcement):

✅ CODE QUALITY (OLD: requirements)
   • Builds cleanly on CI (Clang-22, MSVC)
   • No compiler warnings (warnings-as-errors)
   • Sanitizers green (UB, memory leaks)
   • Follows CODING_STYLE.md

✅ TESTING (OLD: metrics)
   • Unit tests added/updated
   • Coverage ≥ 85% on touched lines
   • All tests pass and deterministic
   • Integration tests for cross-module

✅ DOCUMENTATION (OLD: same-PR requirement)
   • API docs updated (inline comments)
   • Module README updated if needed
   • Examples compile and work
   • Task file status updated

✅ PERFORMANCE (OLD: requirement)
   • Benchmarks run (no regressions > 5%)
   • Telemetry integrated
   • Tracy zones on hot paths

✅ REVIEW (NEW: explicit validation)
   • Code reviewed by Reviewer role
   • All quality gates passed
   • No outstanding comments
   • PR template completed

┌─────────────────────────────────────────────────────────────────────────┐
│                         SUCCESS METRICS                                 │
└─────────────────────────────────────────────────────────────────────────┘

EFFECTIVENESS (NEW: formalized tracking)
• Time to first contribution: < 30 minutes
• Agent autonomy: ≥ 80% completion rate
• Documentation freshness: 100% of PRs
• Hand-off success: Zero context loss

QUALITY (OLD: maintained standards)
• DoD compliance: 100% of merged PRs
• Performance regressions: < 5%
• Test coverage: ≥ 85% on touched code
• Documentation accuracy: Zero broken links

VELOCITY (NEW: efficiency tracking)
• PR cycle time: < 48 hours
• Rework rate: < 15%
• Automation rate: ≥ 70%

┌─────────────────────────────────────────────────────────────────────────┐
│                    CONTINUOUS IMPROVEMENT                               │
└─────────────────────────────────────────────────────────────────────────┘

🔄 Auto-Improver (14) - NEW: dedicated improvement role
   • Monitors workflow metrics
   • Proposes process improvements
   • Refactors documentation based on usage

👥 Community Maintainer (16) - NEW: community feedback
   • Gathers contributor feedback
   • Identifies friction points
   • Improves onboarding

Legend:
(OLD) = Element from original workflow
(NEW) = Element from new multi-agent workflow
HYBRID = Best of both combined
```

