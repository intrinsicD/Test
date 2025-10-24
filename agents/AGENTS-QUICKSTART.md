# AGENTS-QUICKSTART.md

## 🎯 Essential First Steps

**Before starting any task:**
1. Read [README.md](../README.md) - workspace snapshot and module health
2. Review [docs/NAVIGATION.md](../docs/NAVIGATION.md) - documentation structure
3. Check [CODING_STYLE.md](../CODING_STYLE.md) - coding standards
4. Skim [agents/AGENTS.md](AGENTS.md) - coordination loop and hand-off expectations
5. Understand the build workflow (see section below)

---

## 📋 Task Workflow

### 1) **Create an Issue or Task**

**For internal/active tasks:** Create `docs/tasks/<MODULE>-<NUMBER>.md` from [`TEMPLATES/TASK_CARD.md`](TEMPLATES/TASK_CARD.md)

**For community/external:** Create GitHub Issue from [`TEMPLATES/ISSUE_TEMPLATE.md`](TEMPLATES/ISSUE_TEMPLATE.md).  
   - Describe the problem, scope, and acceptance tests.
   - Link to any relevant ADRs or roadmap items.

2) **Architect writes an ADR** from [`TEMPLATES/ADR_TEMPLATE.md`](TEMPLATES/ADR_TEMPLATE.md).  
   - Define context, decision, and consequences.
   - Store under `docs/adr/`.

3) **Orchestrator posts a CONTEXT PACK** to the chosen role (Engineer, Lead, etc.).  
   - Use [`TEMPLATES/CONTEXT_PACK.md`](TEMPLATES/CONTEXT_PACK.md) to provide:
     - Relevant file excerpts
     - Build/test commands
     - Open issues or PRs
     - Acceptance tests and benchmarks

4) **Role executes the task**  
   - Follow the Codex loop: **Read → Plan → Patch (diffs) → Tests/Docs → Report.**
   - Keep diffs under 400 LOC.
   - Add unit tests, benchmarks, and update docs in the same PR.
## 🏗️ Build & Test Workflow

### Configure
```bash
cmake --preset <debug|release|sanitize>
```

### Build
```bash
cmake --build --preset <debug|release|sanitize>
```

### Test
```bash
ctest --preset <debug|release|sanitize> --output-on-failure
pytest python/tests scripts/tests
python scripts/validate_docs.py
```

**Available Presets:**
- `debug` - Debug build with assertions
- `release` - Optimized release build
- `sanitize` - Debug with ASan/UBSan

See [README.md section 3](../README.md) for complete build instructions.

---


5) **Reviewer, Performance Engineer, and Security Gate** check the PR.  
    > README → NAVIGATION → Issue/Task → ADR → Context Pack → Implementation → Review → Release  
     - ✅ Builds on CI (Clang-22/libc++ & MSVC)
     - ✅ Tests ≥85% coverage on touched code
     - ✅ No perf regression (>2%)
     - ✅ Docs and examples updated
     - ✅ Logging, tracing, and assertions present

6) **Release Manager ships** on the regular cadence.  
   - Merge PRs when green.
   - Update release notes and changelogs.
   - Tag, package, and publish artifacts.

---

**Summary:**

    > Issue → ADR → Context Pack → Implementation → Review → Release  
    This process ensures reproducibility, quality, and steady improvement across all agents working on the 3D engine.
