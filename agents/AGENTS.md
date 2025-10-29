# Agent Directory Overview

This directory now hosts the focused artefacts that support the workflow defined in [`../AGENT_WORKFLOW.md`](../AGENT_WORKFLOW.md).

## Start Here
1. Load the portal: [`../AGENT_WORKFLOW.md`](../AGENT_WORKFLOW.md).
2. Identify your responsibilities in [`ROLES.md`](ROLES.md).
3. Use the templates under [`TEMPLATES/`](TEMPLATES) to document briefs, context packs, and quality reports.

## Maintenance Rules
- Keep links to backlog entries, ADRs, and module documentation accurate.
- Update `ROLES.md` and the templates in the same change when responsibilities shift.
- Run `python ../scripts/update_agents_tree.py` after adding or removing artefacts so repository guidance stays synchronized.

Legacy role files and hybrid workflow documents were removed in favour of the streamlined assets above. Refer to repository history if archival context is required.
