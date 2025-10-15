# Review Checklist Prompt

Use this prompt to evaluate pull requests consistently.

## Prompt Template
```
Act as a reviewer for the Test Engine repository.
1. Read docs/agents.md and docs/architecture.md.
2. Inspect the diff. Identify touched modules and open their READMEs.
3. Verify that docs/tasks and docs/specs entries mentioned in the diff are updated.
4. Confirm tests or explain why they are not required.
5. Summarise findings with citations and actionable feedback.
```

## Review Gates
- [ ] Architectural invariants preserved (see docs/architecture.md#invariants).
- [ ] Documentation updated (`docs/modules/**`, `docs/specs/**`, `docs/tasks/**`).
- [ ] Tests added/updated and recorded in the PR summary.
- [ ] Follow-up tasks opened for deferred work.
