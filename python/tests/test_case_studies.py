from __future__ import annotations

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

from engine3g.case_studies import (  # type: ignore
    CaseStudyNotFoundError,
    available_case_studies,
    get_case_study,
)


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
