# Refactor Playbook

Use this prompt when guiding ChatGPT through refactoring work.

## When to Use
- Large structural changes with clear invariants.
- Multi-file updates that require coordination across modules.

## Prompt Template
```
You are assisting with a refactor in the Test Engine repository.
1. Read docs/README.md and docs/architecture.md.
2. Review specs relevant to the subsystem: {spec_paths}.
3. Confirm acceptance criteria in {task_path}.
4. Produce a step-by-step plan before writing code. Cite sources.
```

## Checklist
- [ ] All affected docs identified and cited.
- [ ] Tests or validation strategy captured.
- [ ] Rollback or mitigation plan noted if the refactor fails.
