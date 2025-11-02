"""Utilities to load the modular 3G engine libraries from Python."""

from __future__ import annotations

from contextlib import contextmanager
from dataclasses import dataclass, field
import ctypes
import json
import os
import sys
from pathlib import Path
from types import MappingProxyType
from typing import (
    Any,
    Dict,
    Iterable,
    Iterator,
    List,
    Mapping,
    MutableMapping,
    Optional,
    Sequence,
    Tuple,
)

__all__ = [
    "EngineLibraryNotFound",
    "EngineModuleHandle",
    "EngineRuntimeHandle",
    "RuntimeSession",
    "load_all_modules",
    "load_module",
    "load_runtime",
    "runtime_session",
]


class EngineLibraryNotFound(RuntimeError):
    """Raised when a requested engine library cannot be located."""

    def __init__(
        self,
        identifier: str,
        library_name: str,
        attempted_paths: Sequence[Path],
        last_error: Optional[OSError],
    ) -> None:
        self.identifier = identifier
        self.library_name = library_name
        self.attempted_paths = tuple(Path(path) for path in attempted_paths)
        self.last_error = last_error
        search_hint = ", ".join(str(path) for path in self.attempted_paths) or "<none>"
        message = (
            f"Unable to locate the shared library '{library_name}'. "
            "Set ENGINE3G_LIBRARY_PATH or provide explicit search paths. "
            f"Looked in: {search_hint}."
        )
        super().__init__(message)


class _ResearchRenderingOptions(ctypes.Structure):
    """ctypes mirror of ``engine_runtime_research_rendering_options``."""

    _fields_ = [
        ("width", ctypes.c_uint32),
        ("height", ctypes.c_uint32),
        ("shading_mode", ctypes.c_int),
        ("overlay_normals", ctypes.c_uint8),
        ("overlay_uv", ctypes.c_uint8),
        ("overlay_material", ctypes.c_uint8),
        ("overlay_light_volume", ctypes.c_uint8),
    ]


@dataclass
class EngineModuleHandle:
    """Represents a loaded engine module shared library."""

    name: str
    identifier: str
    library: ctypes.CDLL
    _compatibility_metadata: Optional[Mapping[str, object]] = field(
        default=None, init=False, repr=False
    )

    def resolved_name(self) -> str:
        """Return the authoritative module name as exported by the library."""
        symbol = f"{self.identifier}_module_name"
        func = getattr(self.library, symbol)
        func.restype = ctypes.c_char_p
        result = func()
        return result.decode("utf-8") if result else ""

    def compatibility_metadata(self) -> Mapping[str, object]:
        """Return ABI/compatibility metadata exposed by the module."""

        if self._compatibility_metadata is None:
            metadata = _load_module_compatibility_metadata(
                self.library, self.identifier
            )
            self._compatibility_metadata = MappingProxyType(metadata)
        return self._compatibility_metadata


def _module_metadata_symbol_candidates(identifier: str) -> List[str]:
    return [
        f"{identifier}_module_metadata_json",
        f"{identifier}_module_metadata",
    ]


def _load_module_compatibility_metadata(
    library: ctypes.CDLL, identifier: str
) -> Dict[str, object]:
    metadata: Dict[str, object] = {}
    for symbol_name in _module_metadata_symbol_candidates(identifier):
        func = getattr(library, symbol_name, None)
        if func is None:
            continue
        try:
            func.restype = ctypes.c_char_p
        except AttributeError:
            continue
        raw_value = func()
        if not raw_value:
            return {}
        try:
            decoded = raw_value.decode("utf-8")
        except UnicodeDecodeError as error:
            raise ValueError(
                f"Module '{identifier}' returned non-UTF-8 compatibility metadata"
            ) from error
        try:
            parsed: Any = json.loads(decoded)
        except json.JSONDecodeError as error:
            raise ValueError(
                f"Module '{identifier}' returned invalid JSON compatibility metadata"
            ) from error
        if not isinstance(parsed, Mapping):
            raise ValueError(
                f"Module '{identifier}' metadata must decode to a mapping"
            )
        normalized: Dict[str, object] = {}
        for key, value in parsed.items():
            if not isinstance(key, str):
                raise ValueError(
                    f"Module '{identifier}' metadata keys must be strings"
                )
            normalized[key] = value
        metadata = normalized
        break
    return metadata


class EngineRuntimeHandle:
    """Access to the engine runtime aggregate library."""

    def __init__(self, library: ctypes.CDLL) -> None:
        self.library = library
        self.library.engine_runtime_module_name.restype = ctypes.c_char_p
        self.library.engine_runtime_module_count.restype = ctypes.c_size_t
        self.library.engine_runtime_module_at.restype = ctypes.c_char_p
        self.library.engine_runtime_initialize.restype = None
        self.library.engine_runtime_initialize.argtypes = []
        self.library.engine_runtime_shutdown.restype = None
        self.library.engine_runtime_shutdown.argtypes = []
        self.library.engine_runtime_tick.restype = None
        self.library.engine_runtime_tick.argtypes = [ctypes.c_double]
        self.library.engine_runtime_body_count.restype = ctypes.c_size_t
        self.library.engine_runtime_body_count.argtypes = []
        self.library.engine_runtime_body_position.restype = None
        self.library.engine_runtime_body_position.argtypes = [
            ctypes.c_size_t,
            ctypes.POINTER(ctypes.c_float),
        ]
        self.library.engine_runtime_joint_count.restype = ctypes.c_size_t
        self.library.engine_runtime_joint_count.argtypes = []
        self.library.engine_runtime_joint_name.restype = ctypes.c_char_p
        self.library.engine_runtime_joint_name.argtypes = [ctypes.c_size_t]
        self.library.engine_runtime_joint_translation.restype = None
        self.library.engine_runtime_joint_translation.argtypes = [
            ctypes.c_size_t,
            ctypes.POINTER(ctypes.c_float),
        ]
        self.library.engine_runtime_mesh_bounds.restype = None
        self.library.engine_runtime_mesh_bounds.argtypes = [
            ctypes.POINTER(ctypes.c_float),
            ctypes.POINTER(ctypes.c_float),
        ]
        self.library.engine_runtime_dispatch_count.restype = ctypes.c_size_t
        self.library.engine_runtime_dispatch_count.argtypes = []
        self.library.engine_runtime_dispatch_name.restype = ctypes.c_char_p
        self.library.engine_runtime_dispatch_name.argtypes = [ctypes.c_size_t]
        self.library.engine_runtime_dispatch_duration.restype = ctypes.c_double
        self.library.engine_runtime_dispatch_duration.argtypes = [ctypes.c_size_t]
        self.library.engine_runtime_scene_node_count.restype = ctypes.c_size_t
        self.library.engine_runtime_scene_node_count.argtypes = []
        self.library.engine_runtime_scene_node_name.restype = ctypes.c_char_p
        self.library.engine_runtime_scene_node_name.argtypes = [ctypes.c_size_t]
        self.library.engine_runtime_scene_node_transform.restype = None
        self.library.engine_runtime_scene_node_transform.argtypes = [
            ctypes.c_size_t,
            ctypes.POINTER(ctypes.c_float),
            ctypes.POINTER(ctypes.c_float),
            ctypes.POINTER(ctypes.c_float),
        ]
        self._supports_average_tick_ms = hasattr(
            self.library, "engine_runtime_diagnostic_average_tick_ms"
        )
        if self._supports_average_tick_ms:
            self.library.engine_runtime_diagnostic_average_tick_ms.restype = ctypes.c_double
            self.library.engine_runtime_diagnostic_average_tick_ms.argtypes = []
        configure_rendering = getattr(library, "engine_runtime_configure_research_rendering", None)
        if configure_rendering is not None:
            configure_rendering.restype = None
            configure_rendering.argtypes = [ctypes.POINTER(_ResearchRenderingOptions)]
        self._configure_research_rendering = configure_rendering
        self._is_initialized: bool = False
        self._context_owns_runtime: bool = False

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

    def initialize(self) -> None:
        """Ensure the native runtime is initialized."""
        self.library.engine_runtime_initialize()
        self._is_initialized = True

    def shutdown(self) -> None:
        """Request that the runtime release all cached state."""
        self.library.engine_runtime_shutdown()
        self._is_initialized = False

    def tick(self, dt: float) -> None:
        """Advance the runtime simulation by ``dt`` seconds."""
        self.library.engine_runtime_tick(ctypes.c_double(dt))

    def __enter__(self) -> EngineRuntimeHandle:
        """Activate the runtime when used as a context manager."""
        if not self._is_initialized:
            self.initialize()
            self._context_owns_runtime = True
        else:
            self._context_owns_runtime = False
        return self

    def __exit__(self, exc_type, exc_value, traceback) -> None:
        """Shut down the runtime on context manager exit."""
        try:
            if self._context_owns_runtime:
                self.shutdown()
        finally:
            self._context_owns_runtime = False

    def mesh_bounds(self) -> Tuple[Tuple[float, float, float], Tuple[float, float, float]]:
        """Return the axis-aligned bounds of the deformed mesh."""
        mins = (ctypes.c_float * 3)()
        maxs = (ctypes.c_float * 3)()
        self.library.engine_runtime_mesh_bounds(mins, maxs)
        return (
            (float(mins[0]), float(mins[1]), float(mins[2])),
            (float(maxs[0]), float(maxs[1]), float(maxs[2])),
        )

    def body_positions(self) -> List[Tuple[float, float, float]]:
        """Return simulated rigid body positions from the physics world."""
        count = int(self.library.engine_runtime_body_count())
        vector = (ctypes.c_float * 3)()
        positions: List[Tuple[float, float, float]] = []
        for index in range(count):
            self.library.engine_runtime_body_position(index, vector)
            positions.append((float(vector[0]), float(vector[1]), float(vector[2])))
        return positions

    def configure_research_rendering(
        self,
        *,
        shading_mode: str,
        width: int,
        height: int,
        overlays: Mapping[str, bool],
    ) -> None:
        """Configure research rendering options exposed by the runtime."""

        if self._configure_research_rendering is None:
            raise RuntimeError("runtime library does not expose research rendering configuration")
        if width <= 0 or height <= 0:
            raise ValueError("width and height must be positive integers")

        shading_value = shading_mode.strip().lower()
        if shading_value not in {"forward", "deferred"}:
            raise ValueError("shading_mode must be 'forward' or 'deferred'")

        options = _ResearchRenderingOptions()
        options.width = int(width)
        options.height = int(height)
        options.shading_mode = 0 if shading_value == "forward" else 1
        options.overlay_normals = 1 if overlays.get("normals", False) else 0
        options.overlay_uv = 1 if overlays.get("uv", False) else 0
        options.overlay_material = 1 if overlays.get("material", False) else 0
        options.overlay_light_volume = 1 if overlays.get("light_volume", False) else 0
        # ``ctypes.byref`` returns a lightweight proxy that lacks the ``contents``
        # attribute expected by our tests and harness diagnostics. ``pointer``
        # materialises a stable pointer instance whose lifetime is bound to the
        # ``options`` structure, keeping behaviour consistent for both mocks and
        # the native runtime symbol.
        self._configure_research_rendering(ctypes.pointer(options))

    def joint_translations(self) -> Dict[str, Tuple[float, float, float]]:
        """Return joint translations for the current animation pose."""
        count = int(self.library.engine_runtime_joint_count())
        vector = (ctypes.c_float * 3)()
        joints: Dict[str, Tuple[float, float, float]] = {}
        for index in range(count):
            name_ptr = self.library.engine_runtime_joint_name(index)
            if not name_ptr:
                continue
            self.library.engine_runtime_joint_translation(index, vector)
            joints[name_ptr.decode("utf-8")] = (
                float(vector[0]),
                float(vector[1]),
                float(vector[2]),
            )
        return joints

    def dispatch_order(self) -> List[str]:
        """Return the names of kernels executed during the last tick."""
        count = int(self.library.engine_runtime_dispatch_count())
        names: List[str] = []
        for index in range(count):
            name_ptr = self.library.engine_runtime_dispatch_name(index)
            if name_ptr:
                names.append(name_ptr.decode("utf-8"))
        return names

    def dispatch_durations(self) -> List[float]:
        """Return per-kernel durations (in seconds) for the last dispatch."""
        count = int(self.library.engine_runtime_dispatch_count())
        durations: List[float] = []
        for index in range(count):
            durations.append(float(self.library.engine_runtime_dispatch_duration(index)))
        return durations

    def average_tick_ms(self) -> Optional[float]:
        """Return the rolling average tick duration reported by the runtime."""

        if not self._supports_average_tick_ms:
            return None
        return float(self.library.engine_runtime_diagnostic_average_tick_ms())

    def scene_nodes(
        self,
    ) -> List[
        Tuple[
            str,
            Tuple[float, float, float],
            Tuple[float, float, float, float],
            Tuple[float, float, float],
        ]
    ]:
        """Return the scene graph nodes mirrored by the runtime."""
        count = int(self.library.engine_runtime_scene_node_count())
        scales = (ctypes.c_float * 3)()
        rotations = (ctypes.c_float * 4)()
        translations = (ctypes.c_float * 3)()
        nodes: List[
            Tuple[
                str,
                Tuple[float, float, float],
                Tuple[float, float, float, float],
                Tuple[float, float, float],
            ]
        ] = []
        for index in range(count):
            name_ptr = self.library.engine_runtime_scene_node_name(index)
            name = name_ptr.decode("utf-8") if name_ptr else ""
            self.library.engine_runtime_scene_node_transform(
                index,
                scales,
                rotations,
                translations,
            )
            nodes.append(
                (
                    name,
                    (float(scales[0]), float(scales[1]), float(scales[2])),
                    (
                        float(rotations[0]),
                        float(rotations[1]),
                        float(rotations[2]),
                        float(rotations[3]),
                    ),
                    (
                        float(translations[0]),
                        float(translations[1]),
                        float(translations[2]),
                    ),
                )
            )
        return nodes

    def load_modules(
        self, search_paths: Optional[Iterable[os.PathLike[str] | str]] = None
    ) -> Mapping[str, EngineModuleHandle]:
        """Load all registered modules and return them keyed by module name."""
        reusable_paths = _freeze_search_paths(search_paths)
        module_names = self.module_names()

        seen: set[str] = set()
        duplicates: list[str] = []
        for name in module_names:
            if name in seen and name not in duplicates:
                duplicates.append(name)
            else:
                seen.add(name)

        if duplicates:
            search_hint: str
            if reusable_paths is None:
                search_hint = "<none>"
            else:
                search_hint = ", ".join(str(path) for path in reusable_paths) or "<empty>"
            duplicate_list = ", ".join(sorted(duplicates))
            raise ValueError(
                "Runtime reported duplicate module names; all modules must be uniquely "
                "identified before loading. "
                f"Duplicates: {duplicate_list}. Search paths: {search_hint}."
            )

        modules: MutableMapping[str, EngineModuleHandle] = {}
        for name in module_names:
            modules[name] = load_module(name, search_paths=reusable_paths)
        return modules


@dataclass(frozen=True)
class RuntimeSession:
    """Managed runtime scope exposed as a typed helper."""

    runtime: EngineRuntimeHandle
    modules: Mapping[str, EngineModuleHandle]

    def tick(self, dt: float) -> None:
        """Advance the runtime using the managed handle."""

        self.runtime.tick(dt)

    def module(self, name: str) -> EngineModuleHandle:
        """Return the loaded module named *name* or raise ``KeyError``."""

        return self.modules[name]


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
    reusable_paths = _freeze_search_paths(search_paths)
    runtime = load_runtime(search_paths=reusable_paths)
    return runtime.load_modules(search_paths=reusable_paths)


def _canonical_identifier(module_name: str) -> str:
    sanitized = module_name.strip().replace(".", "_")
    if not sanitized:
        raise ValueError("Module name must not be empty")
    if sanitized.startswith("engine_"):
        return sanitized
    return f"engine_{sanitized}"


def _shared_library_name(identifier: str) -> str:
    if sys.platform.startswith("win"):
        return f"{identifier}.dll"
    if sys.platform == "darwin":
        return f"lib{identifier}.dylib"
    return f"lib{identifier}.so"


def _load_shared_library(identifier: str, search_paths: Optional[Iterable[os.PathLike[str] | str]]) -> ctypes.CDLL:
    library_name = _shared_library_name(identifier)
    attempted_paths: List[Path] = []
    last_error: Optional[OSError] = None
    for candidate in _candidate_paths(library_name, search_paths):
        attempted_paths.append(candidate)
        try:
            return ctypes.CDLL(str(candidate))
        except OSError as error:
            last_error = error
            continue
    bare_candidate = Path(library_name)
    try:
        attempted_paths.append(bare_candidate)
        return ctypes.CDLL(library_name)
    except OSError as error:
        last_error = error
    raise EngineLibraryNotFound(
        identifier=identifier,
        library_name=library_name,
        attempted_paths=attempted_paths,
        last_error=last_error,
    ) from last_error


def _candidate_paths(library_name: str, search_paths: Optional[Iterable[os.PathLike[str] | str]]):
    seen: set[Path] = set()
    for base in _iter_search_path_bases(search_paths) + _default_search_paths():
        path = Path(base).expanduser().resolve() / library_name
        if path in seen:
            continue
        seen.add(path)
        yield path


def _freeze_search_paths(
    search_paths: Optional[Iterable[os.PathLike[str] | str]]
) -> Optional[Tuple[os.PathLike[str] | str, ...]]:
    if search_paths is None:
        return None
    if isinstance(search_paths, (str, os.PathLike)):
        return (search_paths,)
    return tuple(search_paths)


def _iter_search_path_bases(
    search_paths: Optional[Iterable[os.PathLike[str] | str]]
) -> List[Path]:
    if search_paths is None:
        return []
    if isinstance(search_paths, (str, os.PathLike)):
        return [Path(search_paths)]
    return [Path(entry) for entry in search_paths]


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


@contextmanager
def runtime_session(
    search_paths: Optional[Iterable[os.PathLike[str] | str]] = None,
    *,
    load_modules: bool = False,
) -> Iterator[RuntimeSession]:
    """Load the runtime, manage its lifetime, and yield a :class:`RuntimeSession`."""

    runtime = load_runtime(search_paths=search_paths)
    modules: Mapping[str, EngineModuleHandle]
    with runtime:
        if load_modules:
            loaded = runtime.load_modules(search_paths=search_paths)
            modules = MappingProxyType(dict(loaded))
        else:
            modules = MappingProxyType({})
        yield RuntimeSession(runtime=runtime, modules=modules)

