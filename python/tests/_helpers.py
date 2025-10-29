"""Shared testing utilities for the Python loader suite."""

from __future__ import annotations

from contextlib import contextmanager
import os
import tempfile
from pathlib import Path
from typing import Iterator, Mapping, MutableMapping

__all__ = [
    "temporary_directory",
    "temporary_env",
]


@contextmanager
def temporary_env(overrides: Mapping[str, str | None]) -> Iterator[MutableMapping[str, str]]:
    """Temporarily update environment variables for the duration of the context."""

    previous: dict[str, str | None] = {key: os.environ.get(key) for key in overrides}
    try:
        for key, value in overrides.items():
            if value is None:
                os.environ.pop(key, None)
            else:
                os.environ[key] = value
        yield os.environ
    finally:
        for key, value in previous.items():
            if value is None:
                os.environ.pop(key, None)
            else:
                os.environ[key] = value


@contextmanager
def temporary_directory(*, prefix: str = "engine-python-tests-") -> Iterator[Path]:
    """Yield a temporary directory as a :class:`Path` and clean it up afterwards."""

    with tempfile.TemporaryDirectory(prefix=prefix) as tmp:
        yield Path(tmp)
