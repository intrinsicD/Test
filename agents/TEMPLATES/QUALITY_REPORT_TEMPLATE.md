# Quality Report Template

> Owner: Quality Gate Leads (QA/Test, Performance, Safety, Docs, Reviewer, Release)

## 1. Task Metadata
- **Task Brief:**
- **Commit / Branch:**
- **Date:**
- **Participants:**
- **Workflow Phase:** (Should be Phase 4 - Quality Gates at submission)
- **Related Artefacts:** (Context package, backlog item, PR link)

## 2. Quality Gate Scorecard *(See [`AGENTS.md` §0.4](../../AGENTS.md#04-phase-checklists))*
| Gate | Owner | Exit Criteria | Evidence | Result | Follow-ups |
| --- | --- | --- | --- | --- | --- |
| Testing |  |  |  | Pass/Fail |  |
| Performance |  |  |  | Pass/Fail |  |
| Security & Safety |  |  |  | Pass/Fail |  |
| Documentation |  |  |  | Pass/Fail |  |
| Review |  |  |  | Pass/Fail |  |
| Release |  |  |  | Pass/Fail |  |

## 3. Command & Evidence Log *(See [`AGENTS.md` §0.5](../../AGENTS.md#05-quality-instrumentation))*
| Command | Result (Pass/Fail) | Log / Artifact | Notes |
| --- | --- | --- | --- |
| `cmake --preset linux-gcc-debug` |  |  |  |
| `cmake --build --preset linux-gcc-debug` |  |  |  |
| `ctest --preset linux-gcc-debug` |  |  |  |
| `pytest python/tests scripts/tests` |  |  |  |
| `python scripts/validate_docs.py` |  |  |  |
| Additional command |  |  |  |

## 4. Metrics & Regression Analysis
- Benchmark deltas:
- Telemetry observations:
- Dataset/version alignment:

## 5. Risk & Regression Notes
- Outstanding risks:
- Mitigations:
- Regression suite updates required:

## 6. Sign-off
| Role | Name | Signature / Timestamp |
| --- | --- | --- |
| QA/Test Specialist |  |  |
| Performance Engineer |  |  |
| Safety Reviewer |  |  |
| Docs/DevRel |  |  |
| Reviewer |  |  |
| Release Manager |  |  |

> Submit this report alongside the PR description or attach it to the task brief. Any unchecked gate blocks merge until resolved.
