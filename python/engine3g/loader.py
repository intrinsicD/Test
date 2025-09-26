"""Utilities to load the modular 3G engine libraries from Python."""

from __future__ import annotations

from dataclasses import dataclass
import ctypes
import os
import sys
from pathlib import Path
from typing import Iterable, List, Mapping, MutableMapping, Optional

__all__ = [
    "EngineLibraryNotFound",
    "EngineModuleHandle",
    "EngineRuntimeHandle",
    "load_all_modules",
    "load_module",
    "load_runtime",
]


class EngineLibraryNotFound(RuntimeError):
    """Raised when a requested engine library cannot be located."""


@dataclass
class EngineModuleHandle:
    """Represents a loaded engine module shared library."""

    name: str
    identifier: str
    library: ctypes.CDLL

    def resolved_name(self) -> str:
        """Return the authoritative module name as exported by the library."""
        symbol = f"{self.identifier}_module_name"
        func = getattr(self.library, symbol)
        func.restype = ctypes.c_char_p
        result = func()
        return result.decode("utf-8") if result else ""


class EngineRuntimeHandle:
    """Access to the engine runtime aggregate library."""

    def __init__(self, library: ctypes.CDLL) -> None:
        self.library = library
        self.library.engine_runtime_module_name.restype = ctypes.c_char_p
        self.library.engine_runtime_module_count.restype = ctypes.c_size_t
        self.library.engine_runtime_module_at.restype = ctypes.c_char_p

    def name(self) -> str:
        """Return the runtime library name."""
        result = self.library.engine_runtime_module_name()
        return result.decode("utf-8") if result else ""

    def module_names(self) -> List[str]:
        """Enumerate module names known to the runtime."""
        count = int(self.library.engine_runtime_module_count())
        names: List[str] = []
        for index in range(count):
            result = self.library.engine_runtime_module_at(index)
            if result:
                names.append(result.decode("utf-8"))
        return names

    def load_modules(self, search_paths: Optional[Iterable[os.PathLike[str] | str]] = None) -> Mapping[str, EngineModuleHandle]:
        """Load all registered modules and return them keyed by module name."""
        modules: MutableMapping[str, EngineModuleHandle] = {}
        for name in self.module_names():
            modules[name] = load_module(name, search_paths=search_paths)
        return modules


def load_runtime(search_paths: Optional[Iterable[os.PathLike[str] | str]] = None) -> EngineRuntimeHandle:
    """Load the aggregate runtime library and return a handle."""
    library = _load_shared_library("engine_runtime", search_paths)
    return EngineRuntimeHandle(library)


def load_module(module_name: str, search_paths: Optional[Iterable[os.PathLike[str] | str]] = None) -> EngineModuleHandle:
    """Load an individual module and return a handle to the shared library."""
    identifier = _canonical_identifier(module_name)
    library = _load_shared_library(identifier, search_paths)
    return EngineModuleHandle(name=module_name, identifier=identifier, library=library)


def load_all_modules(search_paths: Optional[Iterable[os.PathLike[str] | str]] = None) -> Mapping[str, EngineModuleHandle]:
    """Load the runtime and all registered modules in one step."""
    runtime = load_runtime(search_paths=search_paths)
    return runtime.load_modules(search_paths=search_paths)


def _canonical_identifier(module_name: str) -> str:
    sanitized = module_name.replace(".", "_")
    return f"engine_{sanitized}"


def _shared_library_name(identifier: str) -> str:
    if sys.platform.startswith("win"):
        return f"{identifier}.dll"
    if sys.platform == "darwin":
        return f"lib{identifier}.dylib"
    return f"lib{identifier}.so"


def _load_shared_library(identifier: str, search_paths: Optional[Iterable[os.PathLike[str] | str]]) -> ctypes.CDLL:
    library_name = _shared_library_name(identifier)
    for candidate in _candidate_paths(library_name, search_paths):
        try:
            return ctypes.CDLL(str(candidate))
        except OSError:
            continue
    raise EngineLibraryNotFound(
        f"Unable to locate the shared library '{library_name}'. "
        "Set ENGINE3G_LIBRARY_PATH or provide explicit search paths."
    )


def _candidate_paths(library_name: str, search_paths: Optional[Iterable[os.PathLike[str] | str]]):
    seen = set()
    for base in list(search_paths or []) + _default_search_paths():
        path = Path(base).expanduser().resolve() / library_name
        if path in seen:
            continue
        seen.add(path)
        yield path


def _default_search_paths() -> List[Path]:
    paths: List[Path] = []
    env = os.environ.get("ENGINE3G_LIBRARY_PATH")
    if env:
        for entry in env.split(os.pathsep):
            if entry:
                paths.append(Path(entry).expanduser().resolve())
    package_root = Path(__file__).resolve().parent
    paths.append(package_root)
    paths.append(Path.cwd())
    return paths

