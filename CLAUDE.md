# Claude Workflow Notes

## TODO.md / DONE.md Policy

`TODO.md` and `DONE.md` are generated artefacts:

- `TODO.md` contains only active backlog work (`new`, `ready`, `in_progress`, `review`).
- `DONE.md` contains completed backlog history (`done`, `archived`).

Update both files by running:

```bash
python -m scripts.workflow.sync_todo_done
```

This does **not** run automatically on every backlog edit unless you wire the command into your local hooks or CI workflow.
