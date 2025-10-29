#!/bin/bash
# Cleanup redundant documentation files after workflow consolidation
# Date: 2025-10-24

set -e

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$REPO_ROOT"

echo "🧹 Cleaning up redundant documentation files..."

# Create archive directory if it doesn't exist
mkdir -p docs/archive/workflow-migration

# Move historical documentation to archive
echo "📦 Archiving historical restructure documents..."
mv -v docs/DOCUMENTATION_RESTRUCTURE_PROPOSAL.md docs/archive/workflow-migration/ 2>/dev/null || true
mv -v docs/DOCUMENTATION_RESTRUCTURE_CHANGELOG.md docs/archive/workflow-migration/ 2>/dev/null || true
mv -v docs/RESTRUCTURE_SUMMARY.md docs/archive/workflow-migration/ 2>/dev/null || true

# Remove old/redundant AGENTS.md files
echo "🗑️  Removing redundant AGENTS.md files..."
echo "  ℹ️  Skipping removal — AGENTS.md now contains the Agent Directory Workflow guidance."
rm -v agents/README.md || echo "  ⚠️  agents/README.md already removed"

# Optional: Remove README_TEMPLATE.md if unused
# Uncomment the following line if you confirm it's not being used:
# rm -v docs/README_TEMPLATE.md || echo "  ⚠️  README_TEMPLATE.md already removed"

echo ""
echo "✅ Cleanup complete!"
echo ""
echo "📊 Summary:"
echo "  - Archived: 3 historical restructure documents"
echo "  - Removed: agents/README.md (if present)"
echo "  - Retained: AGENTS.md (authoritative manual with agent directory workflow)"
echo ""
echo "🔍 Authoritative workflow artifacts now:"
echo "  - AGENTS.md (single portal, including agent directory workflow)"
echo "  - agents/ROLES.md (responsibility map)"
echo "  - agents/TEMPLATES/ (task, context, quality templates)"
echo ""
echo "💡 Tip: Keep AGENTS.md and the linked templates updated together to"
echo "   avoid documentation drift."
echo ""
echo "🔗 Validating documentation links..."
python scripts/validate_docs.py

