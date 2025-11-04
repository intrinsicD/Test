#!/usr/bin/env python3
"""
Rename all .md files in docs/ to UPPER_SNAKE_CASE following CONTRIBUTION.md standards.
Preserves date prefixes in format YYYY-MM-DD and updates all cross-references.
"""

import os
import re
from pathlib import Path
from typing import Dict, List, Tuple
import argparse


def to_upper_snake_case(filename: str) -> str:
    """Convert filename to UPPER_SNAKE_CASE while preserving date prefixes."""
    # Extract extension
    if not filename.endswith('.md'):
        return filename
    
    name_without_ext = filename[:-3]
    
    # Check for date prefix (YYYY-MM-DD format)
    date_pattern = r'^(\d{4}-\d{2}-\d{2})-(.+)$'
    date_match = re.match(date_pattern, name_without_ext)
    
    if date_match:
        date_prefix = date_match.group(1)
        rest = date_match.group(2)
        # Convert rest to UPPER_SNAKE_CASE
        upper_rest = rest.replace('-', '_').replace('.', '_').upper()
        return f"{date_prefix}-{upper_rest}.md"
    else:
        # No date prefix, convert entire name
        upper_name = name_without_ext.replace('-', '_').replace('.', '_').upper()
        return f"{upper_name}.md"


def find_all_md_files(base_path: Path) -> List[Path]:
    """Find all .md files recursively."""
    return sorted(base_path.rglob("*.md"))


def build_rename_map(md_files: List[Path]) -> Dict[Path, Path]:
    """Build mapping of old paths to new paths."""
    rename_map = {}
    
    for old_path in md_files:
        old_filename = old_path.name
        new_filename = to_upper_snake_case(old_filename)
        
        if old_filename != new_filename:
            new_path = old_path.parent / new_filename
            rename_map[old_path] = new_path
    
    return rename_map


def find_references_in_file(file_path: Path, old_basename: str) -> List[Tuple[int, str]]:
    """Find all lines containing references to the old filename."""
    references = []
    try:
        with open(file_path, 'r', encoding='utf-8') as f:
            for line_num, line in enumerate(f, 1):
                if old_basename in line:
                    references.append((line_num, line.rstrip()))
    except Exception as e:
        print(f"Warning: Could not read {file_path}: {e}")
    
    return references


def update_references_in_file(file_path: Path, rename_map: Dict[Path, Path], dry_run: bool = True) -> int:
    """Update all references in a file. Returns number of updates."""
    try:
        with open(file_path, 'r', encoding='utf-8') as f:
            content = f.read()
    except Exception as e:
        print(f"Warning: Could not read {file_path}: {e}")
        return 0
    
    original_content = content
    updates = 0
    
    # Update references for each renamed file
    for old_path, new_path in rename_map.items():
        old_name = old_path.name
        new_name = new_path.name
        
        if old_name in content:
            content = content.replace(old_name, new_name)
            updates += 1
    
    if content != original_content and not dry_run:
        try:
            with open(file_path, 'w', encoding='utf-8') as f:
                f.write(content)
        except Exception as e:
            print(f"Error: Could not write {file_path}: {e}")
            return 0
    
    return updates


def perform_renames(rename_map: Dict[Path, Path], dry_run: bool = True) -> None:
    """Perform the actual file renames."""
    for old_path, new_path in rename_map.items():
        if dry_run:
            print(f"Would rename: {old_path.relative_to(old_path.parents[len(old_path.parents)-1])} -> {new_path.name}")
        else:
            try:
                old_path.rename(new_path)
                print(f"Renamed: {old_path.name} -> {new_path.name}")
            except Exception as e:
                print(f"Error renaming {old_path}: {e}")


def main():
    parser = argparse.ArgumentParser(description='Rename .md files to UPPER_SNAKE_CASE')
    parser.add_argument('--dry-run', action='store_true', help='Show what would be done without making changes')
    parser.add_argument('--execute', action='store_true', help='Actually perform the renames')
    args = parser.parse_args()
    
    docs_path = Path(__file__).parent.parent / 'docs'
    
    if not docs_path.exists():
        print(f"Error: docs directory not found at {docs_path}")
        return 1
    
    print(f"Scanning {docs_path} for .md files...")
    md_files = find_all_md_files(docs_path)
    print(f"Found {len(md_files)} markdown files")
    
    # Build rename map
    rename_map = build_rename_map(md_files)
    print(f"\nFiles to rename: {len(rename_map)}")
    
    if not rename_map:
        print("No files need renaming. All files already follow UPPER_SNAKE_CASE convention.")
        return 0
    
    # Show rename plan
    print("\n=== RENAME PLAN ===\n")
    for old_path, new_path in sorted(rename_map.items()):
        rel_path = old_path.relative_to(docs_path)
        print(f"  {rel_path}")
        print(f"    → {new_path.name}\n")
    
    if args.dry_run or not args.execute:
        print("\n=== DRY RUN MODE ===")
        print("No changes made. Use --execute to apply changes.")
        print("\nTo execute:")
        print("  python scripts/rename_docs_to_uppercase.py --execute")
        return 0
    
    # Confirm before executing
    print("\n=== EXECUTING RENAMES ===")
    response = input(f"\nRename {len(rename_map)} files? (yes/no): ")
    if response.lower() != 'yes':
        print("Aborted.")
        return 0
    
    # Perform renames
    perform_renames(rename_map, dry_run=False)
    
    # Update cross-references
    print("\n=== UPDATING CROSS-REFERENCES ===")
    all_files = find_all_md_files(docs_path)
    total_updates = 0
    
    for file_path in all_files:
        updates = update_references_in_file(file_path, rename_map, dry_run=False)
        if updates > 0:
            total_updates += updates
            print(f"Updated {updates} reference(s) in {file_path.relative_to(docs_path)}")
    
    print(f"\n=== COMPLETE ===")
    print(f"Renamed {len(rename_map)} files")
    print(f"Updated {total_updates} cross-references")
    print("\nRun: python scripts/validate_docs.py")
    
    return 0


if __name__ == '__main__':
    exit(main())

