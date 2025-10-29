from __future__ import annotations

import json
import sys
from pathlib import Path

import pytest

_TESTS_DIR = Path(__file__).resolve().parent
_PYTHON_ROOT = _TESTS_DIR.parent
_PROJECT_ROOT = _PYTHON_ROOT.parent
if str(_PROJECT_ROOT) not in sys.path:
    sys.path.insert(0, str(_PROJECT_ROOT))
if str(_PYTHON_ROOT) not in sys.path:
    sys.path.insert(0, str(_PYTHON_ROOT))

from engine3g import case_studies as case_studies_module  # type: ignore
from engine3g.case_studies import (  # type: ignore
    CaseStudyError,
    CaseStudyNotFoundError,
    available_case_studies,
    describe_case_studies,
    get_case_study,
)


@pytest.fixture(autouse=True)
def reset_case_study_cache() -> None:
    case_studies_module._case_study_map.cache_clear()
    yield
    case_studies_module._case_study_map.cache_clear()


def test_case_study_registry_lists_available_configs() -> None:
    cases = available_case_studies()
    assert len(cases) >= 2
    for case in cases:
        assert case.config_path.exists(), case.config_path
        assert case.identifier


def test_get_case_study_returns_definition() -> None:
    cases = available_case_studies()
    assert cases, "expected bundled case studies"
    definition = get_case_study(cases[0].identifier)
    assert definition.config_path == cases[0].config_path


def test_get_case_study_unknown_identifier() -> None:
    with pytest.raises(CaseStudyNotFoundError):
        get_case_study("unknown-case-study")


def test_describe_case_studies_exposes_metadata() -> None:
    summaries = describe_case_studies(relative_to=_PROJECT_ROOT)
    assert summaries, "expected case study metadata"
    first = summaries[0]
    assert "id" in first and first["id"]
    assert first["config"].startswith("assets/datasets/case_studies/")
    assert Path(_PROJECT_ROOT, first["config"]).exists()
    assert Path(first["config_absolute"]).exists()
    assert isinstance(first["tags"], list)


def test_case_study_index_requires_schema(monkeypatch: pytest.MonkeyPatch, tmp_path: Path) -> None:
    payload = {"case_studies": []}
    index_path = tmp_path / "index.json"
    index_path.write_text(json.dumps(payload), encoding="utf-8")
    monkeypatch.setattr(case_studies_module, "_index_path", lambda: index_path)

    with pytest.raises(CaseStudyError):
        available_case_studies()


@pytest.mark.parametrize(
    "schema_payload",
    [
        {"schema": {"id": "ai-004.case-studies", "version": 0}},
        {"schema": {"id": "ai-004.case-studies", "version": 2}},
        {"schema": {"id": "ai-004.invalid", "version": 1}},
    ],
)
def test_case_study_index_validates_schema(
    monkeypatch: pytest.MonkeyPatch, tmp_path: Path, schema_payload: dict[str, object]
) -> None:
    payload = dict(schema_payload)
    payload.setdefault("case_studies", [])
    index_path = tmp_path / "index.json"
    index_path.write_text(json.dumps(payload), encoding="utf-8")
    monkeypatch.setattr(case_studies_module, "_index_path", lambda: index_path)

    with pytest.raises(CaseStudyError):
        available_case_studies()
