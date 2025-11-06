# Engine Tools Usage Analysis - Summary

**Date:** 2025-11-06  
**Analysis Document:** `hybrid_workflow/backlog/TOOLS_USAGE_ANALYSIS.md`

---

## Quick Summary

✅ **Finding:** The hybrid_workflow correctly uses **some** engine tools but has gaps in coverage.

### Well-Utilized Tools ✅
- **Experiment Sandbox** — Extensively documented and integrated (AI-004)
- **Benchmark Runners** — Well integrated through prototyping workflows
- **Python automation** — Task status, dashboard generation, harness execution

### Underutilized Tools ⚠️
- **Profiler** — Available but not used in task evidence collection
- **ImGui Helpers** — Limited examples in workflow documentation
- **Panel Registry** — Planned (TL-310) but no follow-up tasks for panels
- **Configuration Loader** — Used implicitly but not documented

---

## Key Findings

### 1. Available Engine Tools (C++)

Located in `engine/tools/include/engine/tools/`:

| Tool | Purpose | Workflow Usage |
|------|---------|----------------|
| **profiling/profiler.hpp** | Performance profiling with PROFILE_SCOPE macro | ⚠️ Not used in quality gates |
| **imgui_helpers.hpp** | Runtime diagnostics UI rendering | ⚠️ Limited examples |
| **imgui/panel_registry.hpp** | Dear ImGui panel management | ✅ Planned in TL-310 |
| **sandbox/experiment_sandbox.hpp** | AI-004 prototyping UI | ✅ Excellent integration |
| **sandbox/configuration_loader.hpp** | JSON configuration parsing | ⚠️ Implicit usage |
| **sandbox/benchmark_runner.hpp** | Automated benchmark execution | ✅ Well integrated |

### 2. Available Python Tools

Located in `scripts/`:

| Tool | Purpose | Workflow Usage |
|------|---------|----------------|
| **prototyping/run_prototype_harness.py** | Headless harness execution | ✅ Active |
| **benchmarks/run_comparative_benchmarks.py** | Comparative benchmarking | ✅ Active |
| **workflow/hybrid_status.py** | Task status reporting | ✅ Active |
| **workflow/dashboard.py** | Dashboard generation | ✅ Active |

### 3. Workflow Coverage Gaps

**Missing from task templates:**
- ❌ Profiler usage examples for performance gates
- ❌ ImGui diagnostic panel creation workflow
- ❌ Benchmark runner integration checklist
- ❌ Configuration validation examples

**Missing follow-up tasks:**
- ❌ No tasks for implementing specific diagnostic panels
- ❌ No tasks for documenting tool integration patterns
- ❌ No tasks for creating tool usage guides

---

## Immediate Recommendations

### Priority 1: Documentation Updates

1. **Update CONTRIBUTING.md** with tool usage patterns:
   ```markdown
   ## Using Profiler for Performance Gates
   
   For tasks with `perf` gate, use PROFILE_SCOPE:
   
   ```cpp
   #include "engine/tools/profiling/profiler.hpp"
   
   void expensive_operation() {
       PROFILE_SCOPE("ExpensiveOperation");
       // ... implementation
   }
   
   // Generate evidence report
   auto report = global_profiler().generate_report();
   ```
   ```

2. **Enhance task template** (`000-template.md`):
   - Add "Tool Integration" section in Design/Plan
   - Include profiling checklist in Evidence section
   - Document benchmark execution requirements

3. **Create tool usage guide** in `docs/modules/tools/WORKFLOWS.md`:
   - Panel creation workflow
   - Profiling integration patterns
   - Benchmark automation guide
   - Configuration validation examples

### Priority 2: Follow-up Tasks

Create tasks to fill panel implementation gaps (TL-310 dependencies):

- **TL-311:** Scene hierarchy diagnostic panel
- **TL-312:** Performance metrics visualization panel
- **TL-313:** Asset browser panel
- **TL-314:** Telemetry visualization panel
- **DC-060:** Document tool integration patterns in CONTRIBUTING.md
- **DC-061:** Enhance task templates with tool checklist

### Priority 3: Quality Gate Integration

Update quality gate criteria to require tool usage:

**Performance Gate:**
- ✅ Require profiler evidence for P1 tasks
- ✅ Include benchmark runner output
- ✅ Document profiling methodology

**Documentation Gate:**
- ✅ Document any new panels in panel registry
- ✅ Update tool module README for new capabilities

---

## Tool Integration Checklist

Quick reference for task authors:

### During Task Planning
- [ ] Review `docs/modules/tools/README.md` for applicable tools
- [ ] Document tool integration in Design/Plan section
- [ ] Add tool-specific evidence requirements to quality gates

### During Implementation  
- [ ] Use `PROFILE_SCOPE` for performance-critical code (if perf gate)
- [ ] Integrate `render_diagnostics()` for UI components
- [ ] Register panels with `PanelRegistry` (if editor component)
- [ ] Use benchmark runners for performance validation

### During Evidence Collection
- [ ] Generate profiler reports for perf gate
- [ ] Capture benchmark runner output
- [ ] Document panel registration
- [ ] Show configuration validation results

---

## Next Steps

1. **Review** this analysis with Product Manager and Tools Lead
2. **Create** follow-up tasks (TL-311 through TL-314, DC-060, DC-061)
3. **Update** CONTRIBUTING.md with tool usage examples
4. **Enhance** task template with tool integration checklist
5. **Document** tool workflows in `docs/modules/tools/WORKFLOWS.md`

---

## Files Created

- `hybrid_workflow/backlog/TOOLS_USAGE_ANALYSIS.md` — Full analysis report
- `TOOLS_USAGE_SUMMARY.md` — This summary (for quick reference)

---

**Status:** ✅ Analysis Complete  
**Next Action:** Review findings and prioritize recommendations  
**Owner:** Product Manager + Tools Lead

