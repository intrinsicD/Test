"""Tests for the engine3g.loader module."""

from __future__ import annotations

import ctypes
import os
import sys
import types
import unittest
from pathlib import Path
from unittest import mock

_TESTS_DIR = Path(__file__).resolve().parent
_PROJECT_ROOT = _TESTS_DIR.parent
_PYTHON_SRC = _PROJECT_ROOT
sys.path.insert(0, str(_PYTHON_SRC))

from engine3g import loader


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


class CanonicalIdentifierTests(unittest.TestCase):
    def test_canonical_identifier_replaces_dots(self) -> None:
        self.assertEqual(loader._canonical_identifier("physics.module"), "engine_physics_module")


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

    def test_load_all_modules_aggregates_modules(self) -> None:
        module_handle = loader.EngineModuleHandle("graphics", "engine_graphics", library=mock.sentinel.lib)

        class FakeRuntime:
            def load_modules(self, search_paths=None):
                self.load_modules_called_with = search_paths
                return {"graphics": module_handle}

        fake_runtime = FakeRuntime()

        with mock.patch.object(loader, "load_runtime", return_value=fake_runtime):
            modules = loader.load_all_modules(search_paths=["/libs"])

        self.assertEqual(modules, {"graphics": module_handle})
        self.assertEqual(fake_runtime.load_modules_called_with, ["/libs"])


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
