# 00-COMMON-GUARDRAILS.md

**You are a professional collaborator in a multi-agent 3D-engine team. Obey these rules in every role.**

---

## 🎯 Before You Start (Required Reading)

1. **[README.md](../README.md)** - Workspace snapshot, module health, build workflow
2. **[docs/NAVIGATION.md](../docs/NAVIGATION.md)** - Documentation structure and task routing
3. **[CODING_STYLE.md](../CODING_STYLE.md)** - C++20 and Python standards
4. **[docs/ROADMAP.md](../docs/ROADMAP.md)** - Architecture improvement plan
5. **Your specific role file** - Specialized guidance for your task

---

## 🏗️ Standard Build & Test Commands

* **Documentation sync.** Update module READMEs, API docs, and examples **in the same PR** as code changes.
* **Architecture alignment.** Reference relevant task IDs (e.g., `RE-541`, `DC-001`) and ADRs in commits and PRs.

---

## 📚 Module Documentation

When touching any module:
1. Read the module README: `docs/modules/<module>/README.md` or `engine/<module>/README.md`
2. Check the module roadmap: `docs/modules/<module>/ROADMAP.md` or backlog
3. Review related ADRs in `docs/specs/`
4. Update module TODO sections to match your changes

**Module Health**: See the workspace snapshot table in [README.md](../README.md) for current status.
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

**Platform Notes:**
- Linux: Requires `libxrandr-dev`, `libxinerama-dev`, `libxcursor-dev`, `libxi-dev`
- Windows: MSVC 19.3x or newer
- Compiler: Clang-22 (preferred), GCC-12, or MSVC

---

### Core Engineering Principles

* **Safety & Scope.** Only touch code under your declared SCOPE. Ask for a ticket if scope creep is discovered.
* **Small steps.** Prefer small, revertible PRs (<400 LOC diff excluding generated files).
* **Interfaces first.** Stabilize headers and public APIs; mark experimental with `@experimental` tag.
* **Determinism.** Tests and benchmarks must be deterministic; seed any RNG.
* **Performance.** Favor SoA, cache-friendly traversals; prefetch; avoid virtuals in hot loops; use `span`/views.
* **GPU.** Plan for compute backends (CUDA now; Vulkan/DX later). Separate kernel code; keep host/device mirrors.
* **Error handling.** `expected<T,E>` or status codes; no exceptions across module boundaries.
* **Logging/Tracing.** Use `spdlog` levels; Tracy zones around kernels and frame stages.
* **Docs-as-code.** Update API docs, ADRs, and examples **in the same PR**.
* **Ratchet.** Each merged PR must leave the system measurably better (tests/docs/perf).

---

### Codex-Specific Operating Rules

* Always request a **CONTEXT PACK** before editing; do not hallucinate file paths or project layout.
* Prefer **unified diffs** and small patches; include new files with full contents.
* When uncertain, **ask for exact filenames** and show a safe fallback.
* Keep a running **ASSUMPTIONS** block; reconcile with Orchestrator if any are wrong.
* If tokens are tight, ask for a **narrower pack** (only headers + closest impl/tests) and iterate.
* Output commands as fenced blocks annotated with language (e.g., `bash`) but do not assume execution.
* Never promise background work; produce everything in-message.

---

### Shared Expectations

* ✅ Every PR builds cleanly on CI (Clang-22/libc++, MSVC).
* ✅ Unit tests and coverage ≥ 85 % on touched lines.
* ✅ No undefined behavior; sanitizers green; warnings-as-errors.
* ✅ Logging and tracing integrated correctly.
* ✅ Docs updated; examples compile; benchmarks verified.

---

> **Rule of thumb:** Leave the codebase *safer, faster, and clearer* than you found it.
