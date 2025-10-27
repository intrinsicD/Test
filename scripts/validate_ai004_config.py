"""Command-line entry point for validating AI-004 dataset and configuration manifests."""

from __future__ import annotations

import argparse
from pathlib import Path
import sys
from typing import Callable, Iterable, Sequence

PROJECT_ROOT = Path(__file__).resolve().parents[1]
PYTHON_ROOT = PROJECT_ROOT / "python"
if str(PROJECT_ROOT) not in sys.path:
    sys.path.insert(0, str(PROJECT_ROOT))
if str(PYTHON_ROOT) not in sys.path:
    sys.path.insert(0, str(PYTHON_ROOT))

from engine3g import (  # type: ignore
    ConfigurationSchemaError,
    load_configuration,
    load_dataset_manifest,
)


Validator = Callable[[Path], object]


def _validate_file(path: Path, description: str, validator: Validator) -> list[str]:
    errors: list[str] = []
    if not path.exists():
        errors.append(f"{description}: {path} does not exist")
        return errors
    try:
        validator(path)
    except ConfigurationSchemaError as exc:  # pragma: no cover - exercised in tests
        errors.append(f"{description}: {path} failed validation: {exc}")
    except Exception as exc:  # pragma: no cover - unexpected failures
        errors.append(f"{description}: {path} raised unexpected error: {exc}")
    return errors


def _validate_datasets(paths: Sequence[Path]) -> list[str]:
    errors: list[str] = []
    for dataset_path in paths:
        errors.extend(_validate_file(dataset_path, "dataset manifest", load_dataset_manifest))
    return errors


def _validate_configurations(paths: Sequence[Path]) -> list[str]:
    errors: list[str] = []
    for config_path in paths:
        errors.extend(_validate_file(config_path, "configuration", load_configuration))
    return errors


def main(argv: Iterable[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--dataset",
        dest="datasets",
        action="append",
        type=Path,
        default=[],
        help="Path to a dataset manifest to validate. Can be specified multiple times.",
    )
    parser.add_argument(
        "--config",
        dest="configs",
        action="append",
        type=Path,
        default=[],
        help="Path to a top-level AI-004 configuration file to validate. Can be specified multiple times.",
    )
    args = parser.parse_args(list(argv) if argv is not None else None)

    if not args.datasets and not args.configs:
        parser.error("At least one --dataset or --config path must be provided.")

    errors: list[str] = []
    errors.extend(_validate_datasets(args.datasets))
    errors.extend(_validate_configurations(args.configs))

    if errors:
        for error in errors:
            print(error, file=sys.stderr)
        return 1

    total = len(args.datasets) + len(args.configs)
    noun = "file" if total == 1 else "files"
    print(f"Validated {total} {noun} successfully.")
    return 0


if __name__ == "__main__":  # pragma: no cover - CLI entry point
    raise SystemExit(main())
