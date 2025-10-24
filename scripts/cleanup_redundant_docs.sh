#!/bin/bash
# Cleanup redundant documentation files after hybrid workflow migration
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
rm -v AGENTS.md || echo "  ⚠️  Root AGENTS.md already removed"
rm -v agents/AGENTS.md || echo "  ⚠️  agents/AGENTS.md already removed"
rm -v agents/README.md || echo "  ⚠️  agents/README.md already removed"

# Optional: Remove README_TEMPLATE.md if unused
# Uncomment the following line if you confirm it's not being used:
# rm -v docs/README_TEMPLATE.md || echo "  ⚠️  README_TEMPLATE.md already removed"

echo ""
echo "✅ Cleanup complete!"
echo ""
echo "📊 Summary:"
echo "  - Archived: 3 historical restructure documents"
echo "  - Removed: 3 redundant AGENTS.md files"
echo ""
echo "🔍 Remaining workflow documents:"
echo "  - docs/HYBRID_WORKFLOW.md (main guide)"
echo "  - docs/HYBRID_WORKFLOW_SUMMARY.md (quick reference)"
echo "  - docs/HYBRID_WORKFLOW_DIAGRAM.md (visual guide)"
echo "  - docs/WORKFLOW_COMPARISON.md (decision rationale)"
echo "  - docs/AGENTIC_WORKFLOW_ENHANCEMENT.md (implementation log)"
echo ""
echo "💡 Consider: The 3 HYBRID_WORKFLOW_* files could be consolidated into one"
echo "   comprehensive document if you want even less clutter."
echo ""
echo "🔗 Validating documentation links..."
python scripts/validate_docs.py

