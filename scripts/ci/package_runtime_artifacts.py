"""Package runtime shared library artefacts for CI pipelines.

This utility copies the runtime shared library (and related debug symbols) into
an artefact directory so downstream jobs and Python diagnostics can locate the
files without relying on environment overrides. It supports both single-config
(presets that emit build artefacts directly under ``out/build/<preset>``) and
multi-config generators which place binaries under configuration subdirectories
(e.g., ``Debug/`` or ``Release/``).
"""

from __future__ import annotations

import argparse
import json
import shutil
import sys
from dataclasses import dataclass
from pathlib import Path
from typing import Iterable, Sequence

REPO_ROOT = Path(__file__).resolve().parents[2]
DEFAULT_ARTIFACT_ROOT = REPO_ROOT / "out" / "artifacts"

RUNTIME_LIBRARY_PATTERNS: tuple[str, ...] = (
    "libengine_runtime*.so",
    "libengine_runtime*.dylib",
    "engine_runtime*.dll",
)
DEBUG_SYMBOL_PATTERNS: tuple[str, ...] = (
    "engine_runtime*.pdb",
    "engine_runtime*.lib",
)


@dataclass(frozen=True)
class PackagingResult:
    """Report detailing the packaged artefacts."""

    build_dir: Path
    output_dir: Path
    artefacts: tuple[Path, ...]

    def to_dict(self) -> dict[str, object]:
        return {
            "build_dir": str(self.build_dir),
            "output_dir": str(self.output_dir),
            "artefacts": [str(path) for path in self.artefacts],
        }


def _collect_matches(build_dir: Path, patterns: Sequence[str]) -> list[Path]:
    matches: list[Path] = []
    for pattern in patterns:
        for candidate in build_dir.rglob(pattern):
            if candidate.is_file():
                matches.append(candidate)
    # Deduplicate while keeping deterministic ordering.
    unique: list[Path] = []
    seen: set[Path] = set()
    for candidate in sorted(matches):
        if candidate in seen:
            continue
        seen.add(candidate)
        unique.append(candidate)
    return unique


def discover_runtime_artefacts(build_dir: Path) -> list[Path]:
    """Locate runtime shared libraries and optional debug symbols.

    Parameters
    ----------
    build_dir:
        Root of the build tree for a given preset.

    Returns
    -------
    list[Path]
        Ordered list of artefact paths.
    """

    if not build_dir.is_dir():
        raise FileNotFoundError(f"build directory {build_dir} does not exist")

    artefacts = _collect_matches(build_dir, RUNTIME_LIBRARY_PATTERNS)
    if not artefacts:
        raise RuntimeError(
            "no runtime shared library artefacts found; build engine_runtime "
            "before packaging"
        )
    artefacts.extend(_collect_matches(build_dir, DEBUG_SYMBOL_PATTERNS))
    return artefacts


def package_runtime_artefacts(
    build_dir: Path,
    output_dir: Path,
    *,
    clean: bool = True,
) -> PackagingResult:
    """Copy runtime artefacts into ``output_dir``.

    The destination directory is created if necessary. Existing files are
    removed when ``clean`` is True to keep CI artefacts deterministic.
    """

    artefacts = discover_runtime_artefacts(build_dir)

    output_dir.mkdir(parents=True, exist_ok=True)
    if clean:
        for existing in output_dir.iterdir():
            if existing.is_file() or existing.is_symlink():
                existing.unlink()
            elif existing.is_dir():
                shutil.rmtree(existing)

    copied: list[Path] = []
    for artefact in artefacts:
        destination = output_dir / artefact.name
        shutil.copy2(artefact, destination)
        copied.append(destination)

    manifest_path = output_dir / "manifest.json"
    with manifest_path.open("w", encoding="utf-8") as manifest_file:
        json.dump(
            {
                "build_dir": str(build_dir.resolve()),
                "artefacts": [path.name for path in copied],
            },
            manifest_file,
            indent=2,
            sort_keys=True,
        )

    copied.append(manifest_path)
    return PackagingResult(build_dir=build_dir, output_dir=output_dir, artefacts=tuple(copied))


def _default_output_dir(preset: str | None) -> Path:
    if preset:
        return DEFAULT_ARTIFACT_ROOT / preset / "runtime"
    return DEFAULT_ARTIFACT_ROOT / "runtime"


def _parse_args(argv: Iterable[str]) -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--preset",
        help=(
            "CMake build preset name. When provided, the build directory is "
            "assumed to live under out/build/<preset>."
        ),
    )
    parser.add_argument(
        "--build-dir",
        type=Path,
        help=(
            "Explicit path to the build directory. Overrides --preset when "
            "specified."
        ),
    )
    parser.add_argument(
        "--output-dir",
        type=Path,
        help=(
            "Destination directory for packaged artefacts. Defaults to "
            "out/artifacts/<preset>/runtime or out/artifacts/runtime when the "
            "preset is omitted."
        ),
    )
    parser.add_argument(
        "--no-clean",
        action="store_true",
        help="Preserve existing files in the output directory instead of deleting them.",
    )
    return parser.parse_args(list(argv))


def main(argv: Iterable[str] | None = None) -> int:
    args = _parse_args(sys.argv[1:] if argv is None else argv)

    if args.build_dir is not None:
        build_dir = args.build_dir
    elif args.preset is not None:
        build_dir = REPO_ROOT / "out" / "build" / args.preset
    else:
        print("error: either --build-dir or --preset must be provided", file=sys.stderr)
        return 1

    output_dir = args.output_dir or _default_output_dir(args.preset)

    try:
        result = package_runtime_artefacts(build_dir, output_dir, clean=not args.no_clean)
    except (FileNotFoundError, RuntimeError) as error:
        print(f"error: {error}", file=sys.stderr)
        return 2

    print(
        "Packaged runtime artefacts:",
        *(path.name for path in result.artefacts if path.name != "manifest.json"),
    )
    print(f"Manifest written to {output_dir / 'manifest.json'}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
