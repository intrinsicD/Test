---
id: ANALYSIS-001
title: Engine Tools Usage Analysis - Hybrid Workflow Coverage
status: done
priority: P3
area: tools
size: S
owner: agent-orchestrator
gates: [docs]
relates_to: [bundle:B, TL-310]
blocked_on: []
links: ["docs/modules/tools/README.md", "hybrid_workflow/backlog/TL-310-editor-foundations.md"]
---

# Analysis: Engine Tools Usage in Hybrid Workflow

**Date:** 2025-11-06  
**Status:** ✅ Complete  
**Scope:** Verify hybrid_workflow tasks reference and utilize all available engine/tools capabilities

---

## Executive Summary

**Finding:** The hybrid_workflow is **partially utilizing** engine/tools capabilities. While several tools are referenced and integrated into workflow planning, **some tools are underutilized or not explicitly mentioned** in task planning and execution.

**Key Gaps Identified:**
1. ✅ **Profiling utilities** — Mentioned in quality gates but not explicitly integrated into task evidence collection
2. ✅ **Panel registry** — Referenced in TL-310 but no tasks document actual panel implementations
3. ⚠️ **ImGui helpers** — Limited direct usage examples in tasks
4. ⚠️ **Configuration loader** — Not explicitly mentioned in workflow automation
5. ✅ **Benchmark runners** — Well integrated through AI-004 and sandbox workflows
6. ✅ **Experiment sandbox** — Extensively documented and integrated

---

## Available Engine Tools Inventory

Based on analysis of `engine/tools/`, the following tools are available:

### 1. **Profiling Tools** (`engine/tools/profiling/`)

**Components:**
- `Profiler` class with begin/end timing
- `ScopedProfile` RAII wrapper
- `PROFILE_SCOPE` macro for automatic profiling
- `ProfileReport` generation with min/max/average statistics
- Global profiler instance

**Current Usage in Hybrid Workflow:**
- ⚠️ **Partially referenced** in quality gates (perf gate mentions benchmarks)
- ❌ **Not explicitly used** in task evidence sections
- ❌ **No examples** of PROFILE_SCOPE usage in task implementation plans

**Recommendation:**
- Add profiling examples to CONTRIBUTING.md
- Include profiler usage in performance gate evidence templates
- Document profiling best practices for P1 tasks

---

### 2. **ImGui Helpers** (`engine/tools/imgui/`)

**Components:**
- `begin_frame()` / `end_frame()` frame management
- `render_diagnostics()` for runtime diagnostics UI
- `render_validation_report()` for scene validation UI
- `render_profiler_window()` for profiling data visualization

**Current Usage in Hybrid Workflow:**
- ✅ **Referenced** in TL-310 for editor foundations
- ⚠️ **Limited examples** in task implementation plans
- ❌ **No tasks** specifically for creating diagnostic panels

**Recommendation:**
- Create task template for "Add diagnostic panel" workflow
- Document ImGui integration patterns in CONTRIBUTING.md
- Add examples of render_diagnostics usage to task evidence sections

---

### 3. **Panel Registry** (`engine/tools/imgui/panel_registry.hpp`)

**Components:**
- `PanelRegistry` class for panel management
- `register_scoped_panel()` RAII helpers plus explicit `register_panel()` / `unregister_panel()` lifecycle APIs
- `render()` / `render_all()` for panel rendering
- `PanelRenderContext` for state forwarding
- Deterministic panel ordering

**Current Usage in Hybrid Workflow:**
- ✅ **Well documented** in TL-310 task
- ✅ **Planned** for implementation with editor foundations
- ⚠️ **No follow-up tasks** for individual panel implementations
- ❌ **Not used** in current workflow automation

**Recommendation:**
- Create follow-up tasks for specific panel implementations:
  - TL-311: Scene hierarchy panel
  - TL-312: Performance metrics panel
  - TL-313: Asset browser panel
  - TL-314: Telemetry visualization panel
- Document panel creation workflow in docs/modules/tools/README.md

---

### 4. **Experiment Sandbox** (`engine/tools/sandbox/`)

**Components:**
- `ExperimentSandbox` main UI class
- `ExperimentConfigurationSummary` configuration model
- Dataset browser with statistics and verification
- Case study preset management
- Rendering controls (presets, shading modes, overlays)
- Benchmark triggers and result display
- Telemetry panel with live metrics
- Programmatic control API (`select_dataset`, `select_rendering_preset`, etc.)
- Persistence (`save_preferences`, `save_layout`)

**Current Usage in Hybrid Workflow:**
- ✅ **Extensively documented** in AI-004 kickoff brief
- ✅ **Well integrated** with case studies and benchmarking
- ✅ **Referenced** in multiple tasks (AI-004, TL-210)
- ✅ **Actively used** for prototyping workflows

**Status:** ✅ **Excellent coverage**

---

### 5. **Configuration Loader** (`engine/tools/sandbox/configuration_loader.hpp`)

**Components:**
- `load_summary_from_json(std::string_view)` for JSON buffer parsing
- `load_summary_from_json(std::filesystem::path)` for file loading
- Integration with harness `--describe-json` output
- Schema validation for experiment configurations

**Current Usage in Hybrid Workflow:**
- ✅ **Used** by experiment sandbox integration
- ⚠️ **Not explicitly mentioned** in workflow automation documentation
- ❌ **No examples** in task evidence collection

**Recommendation:**
- Add configuration loader examples to workflow automation guide
- Document integration with `run_prototype_harness.py --describe-json`
- Include configuration validation in task checklist templates

---

### 6. **Benchmark Runners** (`engine/tools/sandbox/benchmark_runner.hpp`)

**Components:**

#### `PrototypeHarnessBenchmarkRunner`
- Headless harness execution
- Summary file generation and parsing
- Command-line argument construction
- Integration with sandbox preferences

#### `ComparativeBenchmarkRunner`
- Comparative benchmark orchestration
- Integration with `scripts/benchmarks/run_comparative_benchmarks.py`
- Scenario configuration generation
- Result parsing and validation

**Current Usage in Hybrid Workflow:**
- ✅ **Well documented** in docs/modules/tools/README.md
- ✅ **Integrated** with AI-004 benchmarking workflows
- ✅ **Referenced** in quality gates (perf gate)
- ⚠️ **Limited examples** in task evidence sections

**Recommendation:**
- Add benchmark runner examples to task evidence templates
- Document comparative benchmark workflow in CONTRIBUTING.md
- Include benchmark execution checklist in quality gate criteria

---

## Hybrid Workflow Task Coverage Analysis

### Tasks Explicitly Using Engine Tools

| Task ID | Tool(s) Referenced | Integration Level |
|---------|-------------------|-------------------|
| **TL-310** | Panel Registry, ImGui Helpers | ✅ Planned (detailed design) |
| **AI-004** | Experiment Sandbox, Benchmark Runners | ✅ Extensive (kickoff brief) |
| **PM-510** | Sandbox wiring, Benchmark triggers | ✅ Active (integration demos) |
| **T-0120** | *(GPU provider, not tools module)* | N/A |
| **RT-410** | *(Runtime planner, not tools module)* | N/A |
| **RG-450** | *(Render pipeline, not tools module)* | N/A |

### Tasks Missing Tool Integration

**Quality Gate Tasks:**
- ⚠️ No tasks explicitly use `Profiler` for perf gate evidence
- ⚠️ No tasks use `render_diagnostics()` for runtime validation
- ⚠️ No tasks use `configuration_loader` for automated validation

**Documentation Tasks:**
- ❌ No task for documenting profiling best practices
- ❌ No task for documenting ImGui integration patterns
- ❌ No task for panel creation workflow guide

---

## Recommendations

### Immediate Actions (P1)

1. **Update CONTRIBUTING.md** with tool usage examples:
   - Add `PROFILE_SCOPE` usage patterns
   - Document `render_diagnostics()` integration
   - Include benchmark runner workflow

2. **Enhance task templates** (`hybrid_workflow/backlog/000-template.md`):
   - Add profiling evidence section for perf gate
   - Include benchmark execution checklist
   - Document diagnostic panel creation option

3. **Create follow-up tasks for TL-310**:
   - [TL-311](TL-311-scene-hierarchy-panel.md): Implement scene hierarchy diagnostics panel
   - [TL-312](TL-312-performance-metrics-panel.md): Integrate profiler visualization panel
   - [TL-313](TL-313-asset-browser-panel.md): Add asset browser panel
   - [TL-314](TL-314-telemetry-visualization-panel.md): Deliver telemetry visualization panel

### Medium-term Actions (P2)

4. **Document tool integration patterns** in `docs/modules/tools/`:
   - Panel creation guide with examples
   - Profiling integration patterns
   - Benchmark automation workflows
   - Configuration loader usage guide

5. **Add tool usage to quality gates**:
   - Require profiler evidence for perf gate on P1 tasks
   - Encourage diagnostic panel creation for complex features
   - Document benchmark runner usage in perf gate criteria

6. **Create automation helpers**:
   - Script to generate profiler reports from task evidence
   - Helper to validate benchmark configurations
   - Tool to check diagnostic panel registration

### Long-term Actions (P3)

7. **Expand panel registry ecosystem**:
   - Create standard panels for common diagnostics
   - Document panel reuse patterns
   - Build panel library for common workflows

8. **Integrate tools into CI/CD**:
   - Automated profiler report generation
   - Benchmark regression detection
   - Diagnostic validation gates

9. **Tool usage metrics**:
   - Track which tools are used in completed tasks
   - Identify underutilized capabilities
   - Measure tool adoption trends

---

## Tool Usage Checklist

For task authors to reference when planning work:

### Planning Phase
- [ ] Review available tools in `docs/modules/tools/README.md`
- [ ] Identify applicable tools for task area
- [ ] Document tool integration in Design/Plan section
- [ ] Add tool-specific evidence to quality gates

### Implementation Phase
- [ ] Use `PROFILE_SCOPE` for performance-critical sections (if perf gate)
- [ ] Integrate `render_diagnostics()` for runtime visualization (if UI component)
- [ ] Register panels with `PanelRegistry` (if editor component)
- [ ] Use benchmark runners for performance validation (if perf gate)
- [ ] Load configurations with `configuration_loader` (if harness integration)

### Evidence Phase
- [ ] Capture profiler reports for perf gate evidence
- [ ] Include benchmark results with runner output
- [ ] Document panel registration and rendering
- [ ] Show configuration validation output

### Documentation Phase
- [ ] Update tool module README if adding new capabilities
- [ ] Document tool usage patterns in CONTRIBUTING.md
- [ ] Add examples to task evidence templates

---

## Conclusion

**Overall Assessment:** ⚠️ **Good foundation with room for improvement**

The hybrid_workflow has **good coverage** of high-level tools (Experiment Sandbox, Benchmark Runners) but **underutilizes** foundational tools (Profiler, ImGui Helpers, Panel Registry) that could enhance task execution quality and evidence collection.

**Priority Recommendations:**
1. ✅ Update CONTRIBUTING.md with profiling examples
2. ✅ Create TL-311+ follow-up tasks for panel implementations (`TL-311`–`TL-314` now tracked under hybrid_workflow/backlog/)
3. ✅ Enhance task template with tool usage checklist
4. ✅ Document benchmark runner workflow in quality gates

**Next Steps:**
- Review this analysis with Product Manager and Tools Lead
- Prioritize tool integration enhancements in Bundle B
- Update CONTRIBUTING.md and task templates
- Create follow-up tasks for identified gaps

---

## Appendix: Complete Tool Inventory

### C++ Tools (engine/tools/)

| Component | Header | Status | Workflow Usage |
|-----------|--------|--------|----------------|
| Profiler | `profiling/profiler.hpp` | ✅ Implemented | ⚠️ Underutilized |
| ScopedProfile | `profiling/profiler.hpp` | ✅ Implemented | ⚠️ Underutilized |
| ImGui Helpers | `imgui_helpers.hpp` | ✅ Implemented | ⚠️ Partial |
| Panel Registry | `imgui/panel_registry.hpp` | ✅ Implemented | ✅ Planned (TL-310) |
| Experiment Sandbox | `sandbox/experiment_sandbox.hpp` | ✅ Implemented | ✅ Well integrated |
| Configuration Loader | `sandbox/configuration_loader.hpp` | ✅ Implemented | ⚠️ Implicit usage |
| Prototype Benchmark Runner | `sandbox/benchmark_runner.hpp` | ✅ Implemented | ✅ Well integrated |
| Comparative Benchmark Runner | `sandbox/benchmark_runner.hpp` | ✅ Implemented | ✅ Well integrated |

### Python Tools (scripts/)

| Tool | Path | Status | Workflow Usage |
|------|------|--------|----------------|
| Prototype Harness | `prototyping/run_prototype_harness.py` | ✅ Implemented | ✅ Well integrated |
| Comparative Benchmarks | `benchmarks/run_comparative_benchmarks.py` | ✅ Implemented | ✅ Well integrated |
| Task Status | `workflow/hybrid_status.py` | ✅ Implemented | ✅ Active |
| Dashboard Generator | `workflow/dashboard.py` | ✅ Implemented | ✅ Active |

### Planned Tools (TL-310+)

| Tool | Status | Blocking Task |
|------|--------|---------------|
| Editor Application | 🚧 Planned | TL-310 |
| Scene Hierarchy Panel | 📋 Not started | TL-311 (proposed) |
| Performance Metrics Panel | 📋 Not started | TL-312 (proposed) |
| Asset Browser Panel | 📋 Not started | TL-313 (proposed) |
| Telemetry Visualization Panel | 📋 Not started | TL-314 (proposed) |

---

## Evidence

### Analysis Methodology

1. **Tool Inventory:** Scanned `engine/tools/` for all available components
2. **Documentation Review:** Analyzed `docs/modules/tools/README.md` for tool descriptions
3. **Workflow Search:** Grepped `hybrid_workflow/` for tool references
4. **Task Analysis:** Reviewed active and archived tasks for tool integration
5. **Gap Identification:** Compared available tools vs. documented usage

### Files Analyzed

- `engine/tools/CMakeLists.txt`
- `engine/tools/include/engine/tools/**/*.hpp` (all headers)
- `docs/modules/tools/README.md`
- `hybrid_workflow/AGENTS.md`
- `hybrid_workflow/CONTRIBUTING.md`
- `hybrid_workflow/README.md`
- `hybrid_workflow/backlog/*.md` (all task files)

### Quality Gate Sign-offs

| Gate | Status | Owner | Evidence |
|------|--------|-------|----------|
| docs | ✅ Complete | Agent Orchestrator | Analysis document created with comprehensive findings |

---

## Related Work

- **TL-310:** Editor foundations will increase tool usage
- **AI-004:** Experiment sandbox demonstrates good tool integration
- **PM-510:** Weekly demos drive tool adoption
- **Bundle B:** Presentation & tooling bundle tracks tool enablement

**Follow-up Tasks:**
- [ ] Create TL-311: Implement scene hierarchy panel
- [ ] Create TL-312: Implement performance metrics panel  
- [ ] Create TL-313: Implement asset browser panel
- [ ] Create TL-314: Implement telemetry visualization panel
- [ ] Create DC-060: Update CONTRIBUTING.md with tool usage patterns
- [ ] Create DC-061: Enhance task templates with tool checklist

---

**Analysis Completed:** 2025-11-06  
**Analyst:** GitHub Copilot (Agent Orchestrator)  
**Review Status:** Ready for stakeholder review

