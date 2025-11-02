"""Tests for the engine3g.loader module."""

from __future__ import annotations

import ctypes
import json
import os
import sys
import types
import unittest
from pathlib import Path
from unittest import mock

_TESTS_DIR = Path(__file__).resolve().parent
_PROJECT_ROOT = _TESTS_DIR.parent
_PYTHON_SRC = _PROJECT_ROOT
if str(_PYTHON_SRC) not in sys.path:
    sys.path.insert(0, str(_PYTHON_SRC))
if str(_TESTS_DIR) not in sys.path:
    sys.path.insert(0, str(_TESTS_DIR))

from engine3g import loader
from _helpers import temporary_directory, temporary_env


class _DummyFunction:
    """Callable helper that also exposes a ``restype`` attribute."""

    def __init__(self, func):
        self._func = func
        self.restype = None
        self.argtypes = None

    def __call__(self, *args, **kwargs):
        return self._func(*args, **kwargs)


def _make_runtime_namespace(**overrides):
    """Construct a fake runtime namespace with required entry points."""

    defaults = {
        "engine_runtime_module_name": _DummyFunction(lambda: b"runtime"),
        "engine_runtime_module_count": _DummyFunction(lambda: 0),
        "engine_runtime_module_at": _DummyFunction(lambda index: b""),
        "engine_runtime_initialize": _DummyFunction(lambda: None),
        "engine_runtime_shutdown": _DummyFunction(lambda: None),
        "engine_runtime_tick": _DummyFunction(lambda dt: None),
        "engine_runtime_body_count": _DummyFunction(lambda: 0),
        "engine_runtime_body_position": _DummyFunction(lambda index, out: None),
        "engine_runtime_joint_count": _DummyFunction(lambda: 0),
        "engine_runtime_joint_name": _DummyFunction(lambda index: b""),
        "engine_runtime_joint_translation": _DummyFunction(lambda index, out: None),
        "engine_runtime_mesh_bounds": _DummyFunction(lambda mins, maxs: None),
        "engine_runtime_dispatch_count": _DummyFunction(lambda: 0),
        "engine_runtime_dispatch_name": _DummyFunction(lambda index: b""),
        "engine_runtime_dispatch_duration": _DummyFunction(lambda index: 0.0),
        "engine_runtime_scene_node_count": _DummyFunction(lambda: 0),
        "engine_runtime_scene_node_name": _DummyFunction(lambda index: b""),
        "engine_runtime_scene_node_transform": _DummyFunction(
            lambda index, scales, rotations, translations: None
        ),
    }
    defaults.update(overrides)
    return types.SimpleNamespace(**defaults)


def _make_module_handle(
    identifier: str,
    resolved_name: str,
    metadata: dict[str, object] | None = None,
) -> loader.EngineModuleHandle:
    metadata_payload = json.dumps(metadata or {}).encode("utf-8")
    module_name_symbol = _DummyFunction(lambda: resolved_name.encode("utf-8"))
    metadata_symbol = _DummyFunction(lambda: metadata_payload)
    fake_library = types.SimpleNamespace(
        **{
            f"{identifier}_module_name": module_name_symbol,
            f"{identifier}_module_metadata_json": metadata_symbol,
        }
    )
    return loader.EngineModuleHandle(
        name=resolved_name,
        identifier=identifier,
        library=fake_library,
    )


class CanonicalIdentifierTests(unittest.TestCase):
    def test_canonical_identifier_replaces_dots(self) -> None:
        self.assertEqual(loader._canonical_identifier("physics.module"), "engine_physics_module")

    def test_canonical_identifier_accepts_prefixed_names(self) -> None:
        self.assertEqual(loader._canonical_identifier("engine_geometry"), "engine_geometry")

    def test_canonical_identifier_rejects_empty_names(self) -> None:
        with self.assertRaises(ValueError):
            loader._canonical_identifier("   ")


class SharedLibraryNameTests(unittest.TestCase):
    def setUp(self) -> None:
        self._original_platform = sys.platform

    def tearDown(self) -> None:
        sys.platform = self._original_platform

    def test_linux_platform_suffix(self) -> None:
        sys.platform = "linux"
        self.assertEqual(loader._shared_library_name("engine_core"), "libengine_core.so")

    def test_windows_platform_suffix(self) -> None:
        sys.platform = "win32"
        self.assertEqual(loader._shared_library_name("engine_core"), "engine_core.dll")

    def test_macos_platform_suffix(self) -> None:
        sys.platform = "darwin"
        self.assertEqual(loader._shared_library_name("engine_core"), "libengine_core.dylib")


class CandidatePathsTests(unittest.TestCase):
    def test_candidate_paths_merge_and_deduplicate(self) -> None:
        expected_defaults = [Path("/opt/engine"), Path("/usr/local/lib")]
        with mock.patch.object(loader, "_default_search_paths", return_value=expected_defaults):
            search_paths = ["~/custom", "/opt/engine"]
            paths = list(loader._candidate_paths("libengine_core.so", search_paths))
        expanded_custom = Path("~/custom").expanduser().resolve() / "libengine_core.so"
        self.assertEqual(
            paths,
            [
                expanded_custom,
                expected_defaults[0] / "libengine_core.so",
                expected_defaults[1] / "libengine_core.so",
            ],
        )

    def test_candidate_paths_accepts_path_objects(self) -> None:
        base_path = Path("/custom/engine")
        library = "libengine_core.so"
        with mock.patch.object(loader, "_default_search_paths", return_value=[]):
            candidates = list(loader._candidate_paths(library, base_path))

        expected = base_path.expanduser().resolve() / library
        self.assertEqual(candidates, [expected])

    def test_default_search_paths_respects_environment(self) -> None:
        with temporary_directory() as custom_dir:
            other_dir = custom_dir.parent
            env_value = os.pathsep.join([str(custom_dir), str(other_dir)])
            with temporary_env({"ENGINE3G_LIBRARY_PATH": env_value}):
                with mock.patch("pathlib.Path.cwd", return_value=Path("/workspace")):
                    paths = loader._default_search_paths()

        self.assertEqual(paths[0], custom_dir.resolve())
        self.assertEqual(paths[1], other_dir.resolve())
        self.assertEqual(paths[2], Path(loader.__file__).resolve().parent)
        self.assertEqual(paths[3], Path("/workspace"))

    def test_default_search_paths_trims_whitespace_entries(self) -> None:
        with temporary_directory() as base_dir:
            first = base_dir / "lib one"
            second = base_dir / "lib two"
            first.mkdir()
            second.mkdir()
            env_value = os.pathsep.join([f"  {first}  ", "", f"\t{second}\n"])
            with temporary_env({"ENGINE3G_LIBRARY_PATH": env_value}):
                with mock.patch("pathlib.Path.cwd", return_value=Path("/workspace")):
                    paths = loader._default_search_paths()

        self.assertEqual(paths[0], first.resolve())
        self.assertEqual(paths[1], second.resolve())


class LoadSharedLibraryTests(unittest.TestCase):
    def test_load_shared_library_tries_candidates_until_success(self) -> None:
        candidates = [Path("/does/not/exist/libengine_core.so"), Path("/tmp/libengine_core.so")]

        def fake_candidate_paths(name: str, search_paths):
            self.assertEqual(name, "libengine_core.so")
            yield from candidates

        def fake_cdll(path):
            path_obj = Path(path)
            if path_obj == candidates[0]:
                raise OSError("unavailable")
            self.assertEqual(path_obj, candidates[1])
            return mock.sentinel.library

        with mock.patch.object(loader, "_candidate_paths", side_effect=fake_candidate_paths):
            with mock.patch("ctypes.CDLL", side_effect=fake_cdll) as mocked_cdll:
                result = loader._load_shared_library("engine_core", search_paths=None)

        self.assertIs(result, mock.sentinel.library)
        self.assertEqual(mocked_cdll.call_count, 2)

    def test_load_shared_library_raises_when_unavailable(self) -> None:
        candidates = [Path("/missing/libengine_core.so")]

        def fake_candidate_paths(name: str, search_paths):
            yield from candidates

        with mock.patch.object(loader, "_candidate_paths", side_effect=fake_candidate_paths):
            with mock.patch("ctypes.CDLL", side_effect=OSError("missing")):
                with self.assertRaises(loader.EngineLibraryNotFound) as ctx:
                    loader._load_shared_library("engine_core", search_paths=None)

        self.assertIn("libengine_core.so", str(ctx.exception))
        self.assertIn(str(candidates[0]), str(ctx.exception))
        self.assertIn("Last error: missing.", str(ctx.exception))
        self.assertIsInstance(ctx.exception.__cause__, OSError)
        self.assertEqual(str(ctx.exception.__cause__), "missing")
        self.assertEqual(ctx.exception.identifier, "engine_core")
        self.assertEqual(ctx.exception.library_name, "libengine_core.so")
        self.assertEqual(
            ctx.exception.attempted_paths,
            (candidates[0], Path("libengine_core.so")),
        )

    def test_load_shared_library_falls_back_to_bare_name(self) -> None:
        with mock.patch.object(loader, "_candidate_paths", return_value=()):

            def fake_cdll(path: str):
                self.assertEqual(path, "libengine_core.so")
                return mock.sentinel.runtime

            with mock.patch("ctypes.CDLL", side_effect=fake_cdll) as mocked_cdll:
                result = loader._load_shared_library("engine_core", search_paths=None)

        self.assertIs(result, mock.sentinel.runtime)
        mocked_cdll.assert_called_once_with("libengine_core.so")


class HandleBehaviourTests(unittest.TestCase):
    def test_engine_module_handle_resolved_name(self) -> None:
        module_symbol = _DummyFunction(lambda: b"resolved.module")
        fake_library = types.SimpleNamespace(engine_test_module_name=module_symbol)
        handle = loader.EngineModuleHandle(name="test", identifier="engine_test", library=fake_library)
        self.assertEqual(handle.resolved_name(), "resolved.module")
        self.assertIs(module_symbol.restype, ctypes.c_char_p)

    def test_engine_module_handle_resolved_name_handles_null(self) -> None:
        module_symbol = _DummyFunction(lambda: None)
        fake_library = types.SimpleNamespace(engine_test_module_name=module_symbol)
        handle = loader.EngineModuleHandle(name="test", identifier="engine_test", library=fake_library)
        self.assertEqual(handle.resolved_name(), "")
        self.assertIs(module_symbol.restype, ctypes.c_char_p)

    def test_engine_module_handle_metadata_defaults_to_empty(self) -> None:
        fake_library = types.SimpleNamespace(engine_test_module_name=_DummyFunction(lambda: b"test"))
        handle = loader.EngineModuleHandle(name="test", identifier="engine_test", library=fake_library)
        metadata = handle.compatibility_metadata()
        self.assertEqual(dict(metadata), {})
        # cached mapping is reused
        self.assertIs(metadata, handle.compatibility_metadata())

    def test_engine_module_handle_metadata_reads_json_symbol(self) -> None:
        calls: list[None] = []

        def metadata_func() -> bytes:
            calls.append(None)
            payload = {"abi": {"version": "1.2.3"}, "build": "debug"}
            return json.dumps(payload).encode("utf-8")

        fake_library = types.SimpleNamespace(
            engine_test_module_metadata_json=_DummyFunction(metadata_func),
            engine_test_module_name=_DummyFunction(lambda: b"test"),
        )
        handle = loader.EngineModuleHandle(name="test", identifier="engine_test", library=fake_library)

        metadata = handle.compatibility_metadata()
        self.assertEqual(metadata["build"], "debug")
        self.assertEqual(metadata["abi"], {"version": "1.2.3"})
        self.assertIs(fake_library.engine_test_module_metadata_json.restype, ctypes.c_char_p)
        self.assertEqual(len(calls), 1)
        self.assertIs(metadata, handle.compatibility_metadata())
        self.assertEqual(len(calls), 1, "metadata should be cached")

    def test_engine_module_handle_metadata_fallback_symbol(self) -> None:
        fake_library = types.SimpleNamespace(
            engine_test_module_metadata=_DummyFunction(
                lambda: json.dumps({"abi": {"version": 7}}).encode("utf-8")
            ),
            engine_test_module_name=_DummyFunction(lambda: b"test"),
        )
        handle = loader.EngineModuleHandle(name="test", identifier="engine_test", library=fake_library)

        metadata = handle.compatibility_metadata()
        self.assertEqual(metadata["abi"], {"version": 7})
        self.assertIs(fake_library.engine_test_module_metadata.restype, ctypes.c_char_p)

    def test_engine_module_handle_metadata_rejects_non_mapping(self) -> None:
        fake_library = types.SimpleNamespace(
            engine_test_module_metadata_json=_DummyFunction(lambda: b"[1, 2, 3]"),
            engine_test_module_name=_DummyFunction(lambda: b"test"),
        )
        handle = loader.EngineModuleHandle(name="test", identifier="engine_test", library=fake_library)

        with self.assertRaisesRegex(ValueError, "must decode to a mapping"):
            handle.compatibility_metadata()

    def test_engine_module_handle_metadata_invalid_json(self) -> None:
        fake_library = types.SimpleNamespace(
            engine_test_module_metadata_json=_DummyFunction(lambda: b"{invalid"),
            engine_test_module_name=_DummyFunction(lambda: b"test"),
        )
        handle = loader.EngineModuleHandle(name="test", identifier="engine_test", library=fake_library)

        with self.assertRaisesRegex(ValueError, "invalid JSON"):
            handle.compatibility_metadata()

    def test_engine_module_handle_metadata_invalid_utf8(self) -> None:
        fake_library = types.SimpleNamespace(
            engine_test_module_metadata_json=_DummyFunction(lambda: b"\xff"),
            engine_test_module_name=_DummyFunction(lambda: b"test"),
        )
        handle = loader.EngineModuleHandle(name="test", identifier="engine_test", library=fake_library)

        with self.assertRaisesRegex(ValueError, "non-UTF-8"):
            handle.compatibility_metadata()

    def test_collect_module_provenance_resolves_dependencies(self) -> None:
        math_handle = _make_module_handle(
            identifier="engine_math",
            resolved_name="math",
            metadata={"abi": {"version": "1.0"}},
        )
        geometry_handle = _make_module_handle(
            identifier="engine_geometry",
            resolved_name="geometry",
            metadata={"dependencies": ["engine_math"]},
        )

        provenance = loader.collect_module_provenance([geometry_handle, math_handle])

        geometry_record = provenance["engine_geometry"]
        self.assertEqual(geometry_record["name"], "geometry")
        self.assertEqual(geometry_record["metadata"]["dependencies"], ["engine_math"])
        self.assertEqual(len(geometry_record["dependencies"]), 1)
        dependency = geometry_record["dependencies"][0]
        self.assertEqual(dependency["identifier"], "engine_math")
        self.assertEqual(dependency["status"], "resolved")
        self.assertEqual(dependency["name"], "math")
        self.assertEqual(dependency["metadata"], {"abi": {"version": "1.0"}})
        self.assertEqual(dependency["dependencies"], [])

    def test_collect_module_provenance_marks_missing_dependencies(self) -> None:
        tools_handle = _make_module_handle(
            identifier="engine_tools",
            resolved_name="tools",
            metadata={"dependencies": ["engine_rendering"]},
        )

        provenance = loader.collect_module_provenance([tools_handle])

        tools_record = provenance["engine_tools"]
        self.assertEqual(len(tools_record["dependencies"]), 1)
        dependency = tools_record["dependencies"][0]
        self.assertEqual(dependency["identifier"], "engine_rendering")
        self.assertEqual(dependency["status"], "missing")
        self.assertEqual(dependency["unresolved_reason"], "dependency not loaded")
        self.assertNotIn("metadata", dependency)

    def test_collect_module_provenance_detects_cycles(self) -> None:
        module_a = _make_module_handle(
            identifier="engine_a",
            resolved_name="module.a",
            metadata={
                "dependencies": [
                    {"identifier": "engine_b", "version": ">=1.2"},
                ]
            },
        )
        module_b = _make_module_handle(
            identifier="engine_b",
            resolved_name="module.b",
            metadata={"dependencies": ["engine_a"]},
        )

        provenance = loader.collect_module_provenance([module_a, module_b])

        dep_record = provenance["engine_a"]["dependencies"][0]
        self.assertEqual(dep_record["status"], "resolved")
        self.assertEqual(dep_record["requested"], {"version": ">=1.2"})
        self.assertEqual(dep_record["name"], "module.b")
        nested = dep_record["dependencies"][0]
        self.assertEqual(nested["identifier"], "engine_a")
        self.assertEqual(nested["status"], "cycle")
        self.assertIn("cycle detected", nested["unresolved_reason"])
        self.assertEqual(nested["dependencies"], [])

    def test_engine_runtime_handle_exposes_metadata(self) -> None:
        runtime_name = _DummyFunction(lambda: b"runtime")
        module_count = _DummyFunction(lambda: 2)
        module_names = [b"mod.a", b"mod.b"]
        module_at = _DummyFunction(lambda index: module_names[index])
        fake_library = _make_runtime_namespace(
            engine_runtime_module_name=runtime_name,
            engine_runtime_module_count=module_count,
            engine_runtime_module_at=module_at,
        )
        handle = loader.EngineRuntimeHandle(fake_library)
        self.assertEqual(handle.name(), "runtime")
        self.assertEqual(handle.module_names(), ["mod.a", "mod.b"])
        self.assertIs(runtime_name.restype, ctypes.c_char_p)
        self.assertIs(module_count.restype, ctypes.c_size_t)
        self.assertIs(module_at.restype, ctypes.c_char_p)
        self.assertIs(
            fake_library.engine_runtime_dispatch_duration.restype, ctypes.c_double
        )

    def test_engine_runtime_handle_dispatch_durations(self) -> None:
        durations = [0.25, 0.5]

        def fake_duration(index: int) -> float:
            return durations[index]

        fake_library = _make_runtime_namespace(
            engine_runtime_dispatch_count=_DummyFunction(lambda: len(durations)),
            engine_runtime_dispatch_duration=_DummyFunction(fake_duration),
        )
        handle = loader.EngineRuntimeHandle(fake_library)
        self.assertEqual(handle.dispatch_durations(), durations)

    def test_engine_runtime_handle_context_manager_initializes_and_shutdowns(self) -> None:
        events: list[str] = []

        fake_library = _make_runtime_namespace(
            engine_runtime_initialize=_DummyFunction(lambda: events.append("init")),
            engine_runtime_shutdown=_DummyFunction(lambda: events.append("shutdown")),
        )

        handle = loader.EngineRuntimeHandle(fake_library)

        with handle as runtime:
            self.assertIs(runtime, handle)
            self.assertEqual(events, ["init"])

        self.assertEqual(events, ["init", "shutdown"])

        events.clear()
        with handle:
            pass
        self.assertEqual(events, ["init", "shutdown"])

    def test_engine_runtime_handle_context_manager_preserves_external_initialization(self) -> None:
        init_calls: list[str] = []
        shutdown_calls: list[str] = []

        fake_library = _make_runtime_namespace(
            engine_runtime_initialize=_DummyFunction(lambda: init_calls.append("init")),
            engine_runtime_shutdown=_DummyFunction(lambda: shutdown_calls.append("shutdown")),
        )

        handle = loader.EngineRuntimeHandle(fake_library)
        handle.initialize()

        with handle:
            pass

        self.assertEqual(init_calls, ["init"])
        self.assertEqual(shutdown_calls, [])

        handle.shutdown()
        self.assertEqual(shutdown_calls, ["shutdown"])

    def test_engine_runtime_handle_context_manager_shutdowns_on_exception(self) -> None:
        init_calls: list[str] = []
        shutdown_calls: list[str] = []

        fake_library = _make_runtime_namespace(
            engine_runtime_initialize=_DummyFunction(lambda: init_calls.append("init")),
            engine_runtime_shutdown=_DummyFunction(lambda: shutdown_calls.append("shutdown")),
        )

        handle = loader.EngineRuntimeHandle(fake_library)

        with self.assertRaisesRegex(RuntimeError, "boom"):
            with handle:
                raise RuntimeError("boom")

        self.assertEqual(init_calls, ["init"])
        self.assertEqual(shutdown_calls, ["shutdown"])

    def test_engine_runtime_handle_configures_research_rendering(self) -> None:
        captured: dict[str, int] = {}

        def fake_configure(options_ptr):
            self.assertIsNotNone(options_ptr)
            struct = options_ptr.contents
            captured["width"] = struct.width
            captured["height"] = struct.height
            captured["shading_mode"] = struct.shading_mode
            captured["overlay_normals"] = struct.overlay_normals
            captured["overlay_uv"] = struct.overlay_uv
            captured["overlay_material"] = struct.overlay_material
            captured["overlay_light_volume"] = struct.overlay_light_volume

        fake_library = _make_runtime_namespace(
            engine_runtime_configure_research_rendering=_DummyFunction(fake_configure)
        )
        handle = loader.EngineRuntimeHandle(fake_library)

        handle.configure_research_rendering(
            shading_mode="forward",
            width=2048,
            height=1080,
            overlays={"normals": True, "uv": False, "material": True, "light_volume": False},
        )

        self.assertEqual(
            captured,
            {
                "width": 2048,
                "height": 1080,
                "shading_mode": 0,
                "overlay_normals": 1,
                "overlay_uv": 0,
                "overlay_material": 1,
                "overlay_light_volume": 0,
            },
        )

    def test_engine_runtime_handle_configure_research_rendering_requires_symbol(self) -> None:
        handle = loader.EngineRuntimeHandle(_make_runtime_namespace())
        with self.assertRaises(RuntimeError):
            handle.configure_research_rendering(
                shading_mode="deferred", width=1920, height=1080, overlays={}
            )

    def test_engine_runtime_handle_configure_research_rendering_validates_inputs(self) -> None:
        fake_library = _make_runtime_namespace(
            engine_runtime_configure_research_rendering=_DummyFunction(lambda _: None)
        )
        handle = loader.EngineRuntimeHandle(fake_library)

        with self.assertRaises(ValueError):
            handle.configure_research_rendering(
                shading_mode="unknown", width=1920, height=1080, overlays={}
            )

        with self.assertRaises(ValueError):
            handle.configure_research_rendering(
                shading_mode="forward", width=0, height=1080, overlays={}
            )

    def test_engine_runtime_handle_load_modules(self) -> None:
        runtime = loader.EngineRuntimeHandle(
            _make_runtime_namespace(
                engine_runtime_module_count=_DummyFunction(lambda: 1),
                engine_runtime_module_at=_DummyFunction(lambda index: b"graphics"),
            )
        )

        module_handle = loader.EngineModuleHandle("graphics", "engine_graphics", library=mock.sentinel.lib)

        with mock.patch.object(loader, "load_module", return_value=module_handle) as mocked_load_module:
            modules = runtime.load_modules(search_paths=["/libs"])

        mocked_load_module.assert_called_once_with("graphics", search_paths=("/libs",))
        self.assertEqual(modules, {"graphics": module_handle})

    def test_engine_runtime_handle_load_modules_accepts_path(self) -> None:
        runtime = loader.EngineRuntimeHandle(
            _make_runtime_namespace(
                engine_runtime_module_count=_DummyFunction(lambda: 1),
                engine_runtime_module_at=_DummyFunction(lambda index: b"graphics"),
            )
        )

        module_handle = loader.EngineModuleHandle("graphics", "engine_graphics", library=mock.sentinel.lib)
        base_path = Path("/libs")

        with mock.patch.object(loader, "load_module", return_value=module_handle) as mocked_load_module:
            runtime.load_modules(search_paths=base_path)

        mocked_load_module.assert_called_once_with("graphics", search_paths=(base_path,))

    def test_engine_runtime_handle_load_modules_supports_generators(self) -> None:
        runtime = loader.EngineRuntimeHandle(
            _make_runtime_namespace(
                engine_runtime_module_count=_DummyFunction(lambda: 2),
                engine_runtime_module_at=_DummyFunction(
                    lambda index: [b"graphics", b"physics"][index]
                ),
            )
        )

        graphics_handle = loader.EngineModuleHandle(
            "graphics", "engine_graphics", library=mock.sentinel.graphics_lib
        )
        physics_handle = loader.EngineModuleHandle(
            "physics", "engine_physics", library=mock.sentinel.physics_lib
        )

        search_path_values = ["/gen/a", "/gen/b"]
        search_path_iterable = (path for path in search_path_values)

        with mock.patch.object(
            loader,
            "load_module",
            side_effect=[graphics_handle, physics_handle],
        ) as mocked_load_module:
            modules = runtime.load_modules(search_paths=search_path_iterable)

        expected_paths = tuple(search_path_values)
        self.assertEqual(
            mocked_load_module.mock_calls,
            [
                mock.call("graphics", search_paths=expected_paths),
                mock.call("physics", search_paths=expected_paths),
            ],
        )
        self.assertEqual(modules, {"graphics": graphics_handle, "physics": physics_handle})

    def test_engine_runtime_handle_load_modules_rejects_duplicates(self) -> None:
        runtime = loader.EngineRuntimeHandle(
            _make_runtime_namespace(
                engine_runtime_module_count=_DummyFunction(lambda: 2),
                engine_runtime_module_at=_DummyFunction(
                    lambda index: [b"graphics", b"graphics"][index]
                ),
            )
        )

        with mock.patch.object(loader, "load_module") as mocked_load_module:
            with self.assertRaisesRegex(ValueError, "duplicate module names"):
                runtime.load_modules(search_paths=["/libs"])

        mocked_load_module.assert_not_called()

    def test_engine_runtime_handle_filters_null_module_names(self) -> None:
        runtime = loader.EngineRuntimeHandle(
            _make_runtime_namespace(
                engine_runtime_module_count=_DummyFunction(lambda: 3),
                engine_runtime_module_at=_DummyFunction(
                    lambda index: [b"graphics", None, b""][index]
                ),
            )
        )

        self.assertEqual(runtime.module_names(), ["graphics"])


class PublicLoaderHelpersTests(unittest.TestCase):
    def test_load_runtime_returns_handle(self) -> None:
        fake_library = _make_runtime_namespace()

        with mock.patch.object(loader, "_load_shared_library", return_value=fake_library) as mocked_load:
            handle = loader.load_runtime(search_paths=["/libs"])
        mocked_load.assert_called_once()
        self.assertIsInstance(handle, loader.EngineRuntimeHandle)
        self.assertIs(handle.library, fake_library)

    def test_load_module_returns_handle(self) -> None:
        def fake_load(identifier: str, search_paths):
            self.assertEqual(identifier, "engine_rendering_core")
            self.assertEqual(search_paths, ["/libs"])
            return mock.sentinel.module_lib

        with mock.patch.object(loader, "_load_shared_library", side_effect=fake_load):
            handle = loader.load_module("rendering.core", search_paths=["/libs"])

        self.assertIsInstance(handle, loader.EngineModuleHandle)
        self.assertEqual(handle.name, "rendering.core")
        self.assertEqual(handle.identifier, "engine_rendering_core")
        self.assertIs(handle.library, mock.sentinel.module_lib)


class RuntimeSessionTests(unittest.TestCase):
    def test_runtime_session_manages_lifecycle(self) -> None:
        events: list[str] = []

        fake_library = _make_runtime_namespace(
            engine_runtime_initialize=_DummyFunction(lambda: events.append("init")),
            engine_runtime_shutdown=_DummyFunction(lambda: events.append("shutdown")),
            engine_runtime_tick=_DummyFunction(lambda dt: events.append(f"tick:{dt}")),
        )
        runtime_handle = loader.EngineRuntimeHandle(fake_library)

        with mock.patch.object(loader, "load_runtime", return_value=runtime_handle) as mocked_load:
            with loader.runtime_session() as session:
                self.assertIsInstance(session, loader.RuntimeSession)
                self.assertIs(session.runtime, runtime_handle)
                self.assertEqual(dict(session.modules), {})
                session.tick(0.5)
            mocked_load.assert_called_once_with(search_paths=None)

        self.assertEqual(len(events), 3)
        self.assertEqual(events[0], "init")
        self.assertIn("tick", events[1])
        self.assertEqual(events[2], "shutdown")

    def test_runtime_session_optionally_loads_modules(self) -> None:
        runtime_handle = loader.EngineRuntimeHandle(
            _make_runtime_namespace(
                engine_runtime_module_count=_DummyFunction(lambda: 1),
                engine_runtime_module_at=_DummyFunction(lambda index: b"geometry"),
            )
        )
        module_handle = loader.EngineModuleHandle(
            name="geometry",
            identifier="engine_geometry",
            library=mock.sentinel.geometry_lib,
        )

        with mock.patch.object(loader, "load_runtime", return_value=runtime_handle):
            with mock.patch.object(
                runtime_handle,
                "load_modules",
                return_value={"geometry": module_handle},
            ) as mocked_load_modules:
                with loader.runtime_session(search_paths=["/libs"], load_modules=True) as session:
                    self.assertEqual(session.module("geometry"), module_handle)
                    self.assertEqual(set(session.modules.keys()), {"geometry"})

        mocked_load_modules.assert_called_once_with(search_paths=("/libs",))

    def test_runtime_session_freezes_iterable_search_paths(self) -> None:
        runtime_handle = loader.EngineRuntimeHandle(_make_runtime_namespace())
        module_handle = loader.EngineModuleHandle(
            name="geometry",
            identifier="engine_geometry",
            library=mock.sentinel.geometry_lib,
        )

        search_entries = ["/opt/engine", "/usr/local/engine"]
        search_iter = (entry for entry in search_entries)

        with mock.patch.object(loader, "load_runtime", return_value=runtime_handle) as mocked_load_runtime:
            with mock.patch.object(
                runtime_handle,
                "load_modules",
                return_value={"geometry": module_handle},
            ) as mocked_load_modules:
                with loader.runtime_session(search_paths=search_iter, load_modules=True) as session:
                    self.assertEqual(session.module("geometry"), module_handle)
                    self.assertEqual(set(session.modules.keys()), {"geometry"})

        mocked_load_runtime.assert_called_once()
        self.assertEqual(
            mocked_load_runtime.call_args.kwargs["search_paths"],
            tuple(search_entries),
        )
        mocked_load_modules.assert_called_once_with(search_paths=tuple(search_entries))

    def test_load_module_accepts_canonical_identifier(self) -> None:
        with mock.patch.object(
            loader,
            "_load_shared_library",
            side_effect=lambda identifier, search_paths: mock.sentinel.module_lib,
        ) as mocked_load:
            handle = loader.load_module("engine_geometry", search_paths=None)

        mocked_load.assert_called_once_with("engine_geometry", None)
        self.assertEqual(handle.name, "engine_geometry")
        self.assertEqual(handle.identifier, "engine_geometry")
        self.assertIs(handle.library, mock.sentinel.module_lib)

    def test_load_module_rejects_empty_name(self) -> None:
        with self.assertRaises(ValueError):
            loader.load_module("   ", search_paths=None)

    def test_load_all_modules_aggregates_modules(self) -> None:
        module_handle = loader.EngineModuleHandle("graphics", "engine_graphics", library=mock.sentinel.lib)

        class FakeRuntime:
            def load_modules(self, search_paths=None):
                self.load_modules_called_with = search_paths
                return {"graphics": module_handle}

        fake_runtime = FakeRuntime()

        with mock.patch.object(loader, "load_runtime", return_value=fake_runtime) as mocked_load_runtime:
            modules = loader.load_all_modules(search_paths=["/libs"])

        self.assertEqual(modules, {"graphics": module_handle})
        mocked_load_runtime.assert_called_once_with(search_paths=("/libs",))
        self.assertEqual(fake_runtime.load_modules_called_with, ("/libs",))

    def test_load_all_modules_preserves_generator_search_paths(self) -> None:
        module_handle = loader.EngineModuleHandle("graphics", "engine_graphics", library=mock.sentinel.lib)

        runtime_handle = loader.EngineRuntimeHandle(
            _make_runtime_namespace(
                engine_runtime_module_count=_DummyFunction(lambda: 1),
                engine_runtime_module_at=_DummyFunction(lambda index: b"graphics"),
            )
        )

        paths = ["/opt/one", "/opt/two"]
        expected = tuple(paths)
        search_paths = (path for path in paths)

        with mock.patch.object(loader, "load_runtime", return_value=runtime_handle) as mocked_load_runtime:
            with mock.patch.object(loader, "load_module", return_value=module_handle) as mocked_load_module:
                modules = loader.load_all_modules(search_paths=search_paths)

        self.assertEqual(modules, {"graphics": module_handle})
        mocked_load_runtime.assert_called_once_with(search_paths=expected)
        mocked_load_module.assert_called_once_with("graphics", search_paths=expected)


class DefaultSearchPathsTests(unittest.TestCase):
    def test_default_search_paths_include_env_and_package(self) -> None:
        package_root = Path(loader.__file__).resolve().parent
        current_dir = Path.cwd().resolve()

        with mock.patch.dict(os.environ, {"ENGINE3G_LIBRARY_PATH": os.pathsep.join(["~/opt/engine", ""])}, clear=True):
            paths = loader._default_search_paths()

        expected_env = Path("~/opt/engine").expanduser().resolve()
        self.assertIn(expected_env, paths)
        self.assertIn(package_root, paths)
        self.assertIn(current_dir, paths)


if __name__ == "__main__":
    unittest.main()
