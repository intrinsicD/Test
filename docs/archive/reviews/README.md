# Review Sessions Archive

This directory contains historical code review discussions, architectural review sessions, and planning retrospectives from 2025.

## Contents

Files are dated and describe the review focus:
- `2025-02-17-RUNTIME-TELEMETRY.md` - Runtime telemetry review
- `2025-03-17-GEOMETRY-IO-RESULT-MIGRATION.md` - Geometry/IO error handling migration
- `2025-03-22-SCENE-DOCS.md` - Scene documentation review
- `2025-04-05-COMPUTE-CYCLE-DIAGNOSTICS.md` - Compute diagnostics review
- And others...

## Purpose

These reviews captured:
- Architectural discussions and decisions at specific points in time
- Trade-offs considered during feature development
- Action items and follow-up work identified
- Team consensus on technical approaches

## Relationship to ADRs

Some discussions in these reviews led to formal Architecture Decision Records (ADRs) in `../specs/`. When a review resulted in a binding decision, it was documented as an ADR with a permanent ID.

**For binding architectural decisions, always consult `../specs/ADR-*.md`, not these archives.**

## Using These Archives

Consult these reviews when:
- Investigating the history of a decision
- Understanding why a feature was implemented a certain way
- Researching alternatives that were considered but not chosen

**Do not** treat these as current guidance—they capture point-in-time discussions that may have been superseded by later work.

---

**Archive Date:** 2025-10-22  
**Files Archived:** 10 review sessions from February-April 2025
# Implementation Prompts Archive

This directory contains AI conversation logs and implementation-specific prompts from past development sprints (February-April 2025).

## Purpose

These files capture:
- The exact prompts used to guide AI implementation of features
- Iteration cycles and refinements during development
- Implementation patterns that worked well for specific features

## File Naming Convention

Files follow the pattern: `{INITIATIVE-ID}-{DESCRIPTION}.md`

Examples:
- `AI_002_STREAMING_GEOMETRY_TELEMETRY.md` - Async streaming implementation
- `CR_125_CR_130_IMPLEMENTATION.md` - Core module work
- `SC_220_DOCUMENTATION_REFRESH.md` - Scene module docs update

## Current Status

**All files in this directory are historical.** For current implementation guidance, use:
- `../prompts/IMPLEMENTATION_PLAYBOOK.md` - Current standard implementation workflow
- `../prompts/REVIEW_CHECKLIST.md` - Current review process
- Active tasks in `../archive/backlog/legacy/tasks/` - Current work items

## Using These Archives

When working on similar features, you may consult these to:
1. Understand what worked/didn't work in past implementations
2. Learn from iteration patterns
3. Avoid repeating past mistakes

**However:** Always verify that patterns here align with current `CONTRIBUTION.md` and architectural decisions in `../specs/`.

---

**Archive Date:** 2025-10-22  
**Files Archived:** 19 implementation prompts from 2025-02 through 2025-04

