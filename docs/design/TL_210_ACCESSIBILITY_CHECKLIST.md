# TL-210 Experiment Sandbox Accessibility Checklist

**Status:** Draft – update alongside TL-210 feature changes.

Use this checklist whenever the experiment sandbox UI is modified. Each change
should satisfy the checklist before merge and the results should be captured in
code review notes or follow-up issues when exceptions are required.

## Keyboard and Input

- [ ] Every interactive widget (dataset list, preset combos, overlay toggles,
      benchmark buttons) is reachable via keyboard navigation. Provide explicit
      focus targets for custom ImGui composites and avoid intercepting the Tab
      chain unless necessary.
- [ ] Default activation (`Enter`/`Space`) triggers the same callbacks as mouse
      clicks.
- [ ] Scrollable regions expose visible focus indicators when activated by
      keyboard.

## Visual Contrast

- [ ] Text and iconography respect WCAG AA contrast ratios against background
      colours. Prefer Dear ImGui's style helpers to maintain theme consistency.
- [ ] Highlight colours (success/failure states, selection badges) remain legible
      under light/dark themes and colour-blind simulation (protanopia,
      deuteranopia, tritanopia).
- [ ] Do not encode state solely via colour; pair icons or text labels with
      colour changes.

## Messaging and Feedback

- [ ] Benchmark results include textual summaries in addition to colour-coded
      status bars so screen readers can convey success/failure.
- [ ] Warning banners mention the affected selection (`dataset=`, `profile=`)
      when scenarios are auto-fallbacked to help reviewers reconcile state.
- [ ] Toasts and dialogs remain visible long enough for keyboard navigation and
      can be dismissed with `Esc`.

## Screen Reader Semantics

- [ ] Provide `ImGui::SetItemTooltip` or descriptive text for non-labelled
      controls (icon buttons, timeline graphs).
- [ ] Ensure tables expose column headers through `ImGui::TableSetupColumn`
      descriptions so assistive tools can enumerate metrics meaningfully.
- [ ] Label benchmark summary panes with headings so virtual cursor navigation is
      predictable.

## Motion and Animation

- [ ] Avoid gratuitous animation; keep transitions deterministic and respect the
      `ENGINE_UI_REDUCED_MOTION` environment flag when provided.
- [ ] Benchmark progress indicators fall back to textual counters when reduced
      motion is enabled.

## Documentation

- [ ] Update `docs/modules/tools/README.md` and this checklist whenever new UI
      widgets are introduced.
- [ ] Record accessibility verification in PR descriptions, noting any
      outstanding follow-ups.

---

**Runbook:** Re-evaluate this checklist each quarter or when adopting new ImGui
major versions to ensure parity with upstream accessibility changes.
