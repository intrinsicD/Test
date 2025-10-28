"""Registry for AI-004 prototyping case studies packaged with the repository."""

from __future__ import annotations

from dataclasses import dataclass
from functools import lru_cache
import json
from pathlib import Path
from typing import Dict, Tuple

__all__ = [
    "CaseStudy",
    "CaseStudyError",
    "CaseStudyNotFoundError",
    "available_case_studies",
    "get_case_study",
]


class CaseStudyError(RuntimeError):
    """Raised when a case study definition cannot be resolved."""


class CaseStudyNotFoundError(CaseStudyError):
    """Raised when the requested case study identifier is unknown."""


@dataclass(frozen=True)
class CaseStudy:
    """Description of a packaged case study configuration."""

    identifier: str
    label: str
    description: str
    tags: Tuple[str, ...]
    config_path: Path


def _project_root() -> Path:
    return Path(__file__).resolve().parents[2]


def _index_path() -> Path:
    return _project_root() / "assets" / "datasets" / "case_studies" / "index.json"


def _load_index() -> Dict[str, CaseStudy]:
    index_file = _index_path()
    if not index_file.exists():
        return {}

    payload = json.loads(index_file.read_text(encoding="utf-8"))
    case_studies_value = payload.get("case_studies", [])
    if not isinstance(case_studies_value, list):
        raise CaseStudyError("case_studies entry must be a list in case study index")

    base_dir = index_file.parent
    definitions: Dict[str, CaseStudy] = {}
    for entry in case_studies_value:
        if not isinstance(entry, dict):
            raise CaseStudyError("case study entries must be objects")
        identifier = entry.get("id")
        if not isinstance(identifier, str) or not identifier:
            raise CaseStudyError("case study id must be a non-empty string")
        config_rel = entry.get("config")
        if not isinstance(config_rel, str) or not config_rel:
            raise CaseStudyError(f"case study '{identifier}' must define a config path")
        config_path = (base_dir / config_rel).resolve()
        label = entry.get("label", identifier)
        if not isinstance(label, str) or not label:
            label = identifier
        description = entry.get("description", "")
        if not isinstance(description, str):
            raise CaseStudyError(f"case study '{identifier}' description must be a string")
        tags_value = entry.get("tags", [])
        if not isinstance(tags_value, list):
            raise CaseStudyError(f"case study '{identifier}' tags must be a list")
        tags: Tuple[str, ...] = tuple(str(tag) for tag in tags_value)
        definitions[identifier] = CaseStudy(
            identifier=identifier,
            label=label,
            description=description,
            tags=tags,
            config_path=config_path,
        )
    return definitions


@lru_cache(maxsize=1)
def _case_study_map() -> Dict[str, CaseStudy]:
    return _load_index()


def available_case_studies() -> Tuple[CaseStudy, ...]:
    """Return the registered case studies sorted by identifier."""

    existing = [
        case_study
        for case_study in _case_study_map().values()
        if case_study.config_path.exists()
    ]
    return tuple(sorted(existing, key=lambda item: item.identifier))


def get_case_study(identifier: str) -> CaseStudy:
    """Return the case study associated with *identifier* or raise."""

    mapping = _case_study_map()
    try:
        case_study = mapping[identifier]
    except KeyError as error:  # pragma: no cover - defensive guard
        raise CaseStudyNotFoundError(f"unknown case study '{identifier}'") from error
    if not case_study.config_path.exists():
        raise CaseStudyError(
            f"case study '{identifier}' references missing configuration '{case_study.config_path}'"
        )
    return case_study


