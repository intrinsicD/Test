"""Registry for AI-004 prototyping case studies packaged with the repository."""

from __future__ import annotations

from dataclasses import dataclass
from functools import lru_cache
import json
from pathlib import Path
from typing import Dict, Iterable, Tuple

__all__ = [
    "CaseStudy",
    "CaseStudyError",
    "CaseStudyNotFoundError",
    "available_case_studies",
    "describe_case_studies",
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

    def to_dict(self, *, relative_to: Path | None = None) -> Dict[str, object]:
        """Serialise the case study for tooling integrations."""

        config_value = self.config_path
        relative_value: str | None = None
        if relative_to is not None:
            try:
                relative_value = str(config_value.relative_to(relative_to))
            except ValueError:
                relative_value = None

        payload: Dict[str, object] = {
            "id": self.identifier,
            "label": self.label,
            "description": self.description,
            "tags": list(self.tags),
            "config": str(relative_value or config_value),
            "config_absolute": str(config_value),
        }
        return payload


def _project_root() -> Path:
    return Path(__file__).resolve().parents[2]


def _index_path() -> Path:
    return _project_root() / "assets" / "datasets" / "case_studies" / "index.json"


def _load_index() -> Dict[str, CaseStudy]:
    index_file = _index_path()
    if not index_file.exists():
        return {}

    payload = json.loads(index_file.read_text(encoding="utf-8"))
    schema = payload.get("schema")
    if not isinstance(schema, dict):
        raise CaseStudyError("case study index must include a schema header")
    schema_id = schema.get("id")
    if not isinstance(schema_id, str):
        raise CaseStudyError("case study index schema.id must be a string")
    if schema_id != "ai-004.case-studies":
        raise CaseStudyError(
            f"case study index schema.id must be 'ai-004.case-studies'; received '{schema_id}'"
        )
    schema_version = schema.get("version")
    if not isinstance(schema_version, int):
        raise CaseStudyError("case study index schema.version must be an integer")
    if schema_version != 1:
        raise CaseStudyError(
            f"unsupported case study index schema.version '{schema_version}' (expected 1)"
        )

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


def available_case_studies(
    *, include_tags: Iterable[str] | None = None
) -> Tuple[CaseStudy, ...]:
    """Return registered case studies filtered by *include_tags* if provided."""

    required_tags: Tuple[str, ...]
    if include_tags is None:
        required_tags = ()
    else:
        required_tags = tuple(_normalise_tag(tag) for tag in include_tags if tag.strip())

    existing: list[CaseStudy] = []
    for case_study in _case_study_map().values():
        if not case_study.config_path.is_file():
            continue
        if required_tags and not _has_tags(case_study.tags, required_tags):
            continue
        existing.append(case_study)
    return tuple(sorted(existing, key=lambda item: item.identifier))


def get_case_study(identifier: str) -> CaseStudy:
    """Return the case study associated with *identifier* or raise."""

    mapping = _case_study_map()
    try:
        case_study = mapping[identifier]
    except KeyError as error:  # pragma: no cover - defensive guard
        raise CaseStudyNotFoundError(f"unknown case study '{identifier}'") from error
    config_path = case_study.config_path
    if not config_path.exists():
        raise CaseStudyError(
            f"case study '{identifier}' references missing configuration '{config_path}'"
        )
    if not config_path.is_file():
        raise CaseStudyError(
            f"case study '{identifier}' configuration '{config_path}' is not a file"
        )
    return case_study


def describe_case_studies(
    *, relative_to: Path | None = None, include_tags: Iterable[str] | None = None
) -> Tuple[Dict[str, object], ...]:
    """Return case study metadata suitable for UI layers and CLIs."""

    base = relative_to or _project_root()
    cases = available_case_studies(include_tags=include_tags)
    return tuple(case.to_dict(relative_to=base) for case in cases)


def _normalise_tag(value: str) -> str:
    return value.strip().lower()


def _has_tags(tags: Tuple[str, ...], required: Tuple[str, ...]) -> bool:
    available = {_normalise_tag(tag) for tag in tags}
    return all(tag in available for tag in required)


