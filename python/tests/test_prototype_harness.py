from __future__ import annotations

import hashlib
import math
import json
import sys
from dataclasses import dataclass, field
from pathlib import Path
from typing import Callable, List, Optional

import pytest

_TESTS_DIR = Path(__file__).resolve().parent
_PYTHON_ROOT = _TESTS_DIR.parent
_PROJECT_ROOT = _PYTHON_ROOT.parent
if str(_PROJECT_ROOT) not in sys.path:
    sys.path.insert(0, str(_PROJECT_ROOT))
if str(_PYTHON_ROOT) not in sys.path:
    sys.path.insert(0, str(_PYTHON_ROOT))

from engine3g.config_schema import ConfigurationSchemaError, load_configuration
from engine3g.prototype_harness import (
    configuration_summary_to_dict,
    HarnessExecutionOptions,
    HarnessRunSummary,
    PrototypeHarness,
    PrototypeHarnessError,
    run_summary_to_dict,
    load_harness,
    summarize,
)


@dataclass
class _MockRuntime:
    ticks: List[float]
    initialized: bool = False
    shutdown_count: int = 0
    average_tick_value: float = 0.25
    dispatch_names: List[str] = field(default_factory=list)
    dispatch_times: List[float] = field(default_factory=list)
    rendering_configurations: List[dict] = field(default_factory=list)
    configure_exception: Optional[Exception] = None

    def __enter__(self) -> "_MockRuntime":
        self.initialized = True
        return self

    def __exit__(self, exc_type, exc, tb) -> None:
        self.shutdown_count += 1
        self.initialized = False

    def initialize(self) -> None:
        self.initialized = True

    def shutdown(self) -> None:
        self.shutdown_count += 1
        self.initialized = False

    def tick(self, dt: float) -> None:
        self.ticks.append(dt)

    def average_tick_ms(self) -> float:
        return self.average_tick_value

    def dispatch_order(self) -> List[str]:
        return list(self.dispatch_names)

    def dispatch_durations(self) -> List[float]:
        return list(self.dispatch_times)

    def configure_research_rendering(
        self,
        *,
        shading_mode: str,
        width: int,
        height: int,
        overlays,
    ) -> None:
        if self.configure_exception is not None:
            raise self.configure_exception
        self.rendering_configurations.append(
            {
                "shading_mode": shading_mode,
                "width": width,
                "height": height,
                "overlays": dict(overlays),
            }
        )


def _write_configuration(tmp_path: Path) -> Path:
    assets_dir = tmp_path / "assets"
    assets_dir.mkdir(parents=True, exist_ok=True)
    source_mesh = assets_dir / "input.obj"
    output_mesh = assets_dir / "output.obj"
    source_mesh.write_text("v 0 0 0\n", encoding="utf-8")
    output_mesh.write_text("f 1 2 3\n", encoding="utf-8")

    def _sha256(path: Path) -> str:
        digest = hashlib.sha256()
        with path.open("rb") as stream:
            for chunk in iter(lambda: stream.read(65536), b""):
                digest.update(chunk)
        return digest.hexdigest()

    config = {
        "datasets": [
            {
                "schema": {"id": "ai-004.dataset", "version": 2},
                "id": "remesh-sample",
                "kind": "geometry.remesh",
                "tags": ["geometry", "remesh"],
                "source": {
                    "generator": "geometry_remesh",
                    "mesh": source_mesh.relative_to(tmp_path).as_posix(),
                    "mesh_sha256": _sha256(source_mesh),
                    "mesh_size_bytes": source_mesh.stat().st_size,
                },
                "outputs": {
                    "mesh": output_mesh.relative_to(tmp_path).as_posix(),
                    "mesh_sha256": _sha256(output_mesh),
                    "mesh_size_bytes": output_mesh.stat().st_size,
                },
                "remeshing": {"mode": "uniform"},
                "feature_preservation": {
                    "lock_boundary_edges": True,
                    "lock_feature_edges": True,
                    "minimum_feature_angle_degrees": 45.0,
                },
                "parameterization": {
                    "mode": "reuse_existing",
                    "texel_density": 256.0,
                    "target_texel_density": 256.0,
                    "chart_count": 1,
                    "average_stretch": 1.0,
                    "max_stretch": 1.05,
                    "fill_ratio": 0.95,
                    "total_seam_length": 4.0,
                    "charts": [
                        {
                            "index": 0,
                            "min_uv": [0.0, 0.0],
                            "max_uv": [1.0, 1.0],
                            "translation": [0.0, 0.0],
                            "scale": 1.0,
                            "area": 1.0,
                            "boundary_length": 4.0,
                        }
                    ],
                },
                "metrics": {
                    "input": {
                        "vertices": 4,
                        "faces": 2,
                        "edge_length": {"min": 1.0, "max": 1.0, "mean": 1.0},
                    },
                    "output": {
                        "vertices": 4,
                        "faces": 2,
                        "edge_length": {"min": 1.0, "max": 1.0, "mean": 1.0},
                    },
                },
                "statistics": {
                    "iterations": 4,
                    "splits": 1,
                    "collapses": 1,
                    "duration_ms": 1.0,
                    "triangles": 2,
                    "triangle_quality": {"min": 0.8, "mean": 0.9, "max": 0.95},
                    "max_error": 0.1,
                    "min_edge_length": 0.5,
                    "max_edge_length": 1.5,
                    "max_surface_deviation": 0.1,
                    "mean_surface_deviation": 0.05,
                    "rms_surface_deviation": 0.07,
                },
            }
        ],
        "rendering": {
            "schema": {"id": "ai-004.rendering", "version": 1},
            "preset": "research-baseline",
            "options": {"shading_mode": "deferred"},
        },
        "runtime": {
            "schema": {"id": "ai-004.runtime", "version": 2},
            "dataset": "remesh-sample",
            "simulation": {"timestep_seconds": 0.01, "max_substeps": 1},
            "hot_reload": {"enabled": False, "watch_interval_seconds": 0.5},
        },
        "benchmarks": {
            "schema": {"id": "ai-004.benchmarks", "version": 1},
            "scenarios": [
                {
                    "id": "remesh-baseline",
                    "name": "Remesh Baseline",
                    "dataset": "remesh-sample",
                    "rendering_preset": "research-baseline",
                    "engine": {
                        "command": ["python", "run_engine.py", "--output", "{output_path}"],
                        "output": "telemetry/{scenario}_engine.json",
                    },
                    "reference": {
                        "command": ["python", "run_reference.py", "--output", "{output_path}"],
                        "output": "telemetry/{scenario}_reference.json",
                    },
                    "metrics": [
                        {
                            "name": "frame_time",
                            "higher_is_better": False,
                            "threshold": {"type": "relative", "max_regression": 0.05},
                        }
                    ],
                }
            ],
        },
        "telemetry": {
            "schema": {"id": "ai-004.telemetry", "version": 2},
            "outputs": [
                {"type": "file", "path": "telemetry/{scenario}.json"},
                {"type": "stdout"},
            ],
            "metrics": [
                {"name": "frame_time", "statistic": "mean"},
                {"name": "frame_time", "statistic": "p95"},
            ],
            "sampling": {"frame_interval": 8, "include_debug_overlays": True},
        },
    }
    path = tmp_path / "config.json"
    path.write_text(json.dumps(config), encoding="utf-8")
    return path


def test_prototype_harness_executes_ticks(tmp_path: Path) -> None:
    config_path = _write_configuration(tmp_path)
    configuration = load_configuration(config_path)
    runtime = _MockRuntime(
        ticks=[],
        average_tick_value=1.5,
        dispatch_names=["geometry::remesh", "render::composite"],
        dispatch_times=[0.001, 0.0025],
    )

    harness = PrototypeHarness(
        configuration,
        runtime_factory=lambda: runtime,
        asset_search_paths=[tmp_path],
        project_root=tmp_path,
        config_directory=tmp_path,
    )
    summary = harness.run_headless(HarnessExecutionOptions(frames=3, dt=0.5))

    assert runtime.ticks == [0.5, 0.5, 0.5]
    assert runtime.shutdown_count == 1
    assert runtime.rendering_configurations == [
        {
            "shading_mode": "deferred",
            "width": 1920,
            "height": 1080,
            "overlays": {
                "normals": False,
                "uv": False,
                "material": False,
                "light_volume": False,
            },
        }
    ]
    assert summary.frames_executed == 3
    assert summary.dataset_id == "remesh-sample"
    assert summary.rendering_preset == "research-baseline"
    assert summary.dispatch_order == ("geometry::remesh", "render::composite")
    assert summary.dispatch_durations_ms == pytest.approx((1.0, 2.5))
    assert summary.average_tick_ms == pytest.approx(1.5)
    assert tuple(output.kind for output in summary.telemetry_outputs) == ("file", "stdout")
    expected_output_path = (tmp_path / "telemetry" / "remesh-sample.json").resolve()
    assert summary.telemetry_outputs[0].path == str(expected_output_path)
    assert summary.telemetry_outputs[0].template == "telemetry/{scenario}.json"
    assert summary.telemetry_outputs[1].path is None
    assert summary.telemetry_outputs[1].template is None


def test_prototype_harness_reports_rendering_configuration_failures(tmp_path: Path) -> None:
    config_path = _write_configuration(tmp_path)
    configuration = load_configuration(config_path)
    runtime = _MockRuntime(ticks=[], configure_exception=RuntimeError("no support"))

    harness = PrototypeHarness(
        configuration,
        runtime_factory=lambda: runtime,
        asset_search_paths=[tmp_path],
        project_root=tmp_path,
        config_directory=tmp_path,
    )

    with pytest.raises(PrototypeHarnessError) as excinfo:
        harness.run_headless(HarnessExecutionOptions(frames=1, dt=0.5))

    assert "rendering" in str(excinfo.value)


def test_run_headless_supports_custom_scenario_label(tmp_path: Path) -> None:
    config_path = _write_configuration(tmp_path)
    configuration = load_configuration(config_path)

    harness = PrototypeHarness(
        configuration,
        asset_search_paths=[tmp_path],
        project_root=tmp_path,
        config_directory=tmp_path,
    )
    options = HarnessExecutionOptions(dry_run=True, scenario_label="custom-scenario")
    summary = harness.run_headless(options)

    assert summary.scenario_label == "custom-scenario"
    assert summary.telemetry_outputs
    assert summary.telemetry_outputs[0].path is not None
    assert summary.telemetry_outputs[0].path.endswith("telemetry/custom-scenario.json")
    payload = run_summary_to_dict(summary)
    assert payload["scenario"] == "custom-scenario"


def test_run_headless_supports_run_metadata_placeholders(tmp_path: Path) -> None:
    config_path = _write_configuration(tmp_path)
    payload = json.loads(config_path.read_text(encoding="utf-8"))
    payload["telemetry"]["outputs"][0]["path"] = (
        "telemetry/{dataset}-run{run_index:02d}-of-{run_count:02d}.json"
    )
    config_path.write_text(json.dumps(payload), encoding="utf-8")

    configuration = load_configuration(config_path)
    harness = PrototypeHarness(
        configuration,
        asset_search_paths=[tmp_path],
        project_root=tmp_path,
        config_directory=tmp_path,
    )
    options = HarnessExecutionOptions(dry_run=True, run_index=2, run_count=5)
    summary = harness.run_headless(options)

    assert summary.telemetry_outputs
    first_output = summary.telemetry_outputs[0]
    assert first_output.template == (
        "telemetry/{dataset}-run{run_index:02d}-of-{run_count:02d}.json"
    )
    assert first_output.path is not None
    assert first_output.path.endswith("telemetry/remesh-sample-run02-of-05.json")


def test_telemetry_output_sanitizes_scenario_label(tmp_path: Path) -> None:
    config_path = _write_configuration(tmp_path)
    configuration = load_configuration(config_path)

    harness = PrototypeHarness(
        configuration,
        asset_search_paths=[tmp_path],
        project_root=tmp_path,
        config_directory=tmp_path,
    )
    options = HarnessExecutionOptions(dry_run=True, scenario_label="Scenario 01/../..")
    summary = harness.run_headless(options)

    assert summary.telemetry_outputs
    first_output = summary.telemetry_outputs[0]
    assert first_output.path is not None
    assert first_output.path.endswith("telemetry/scenario-01.json")
    assert first_output.template == "telemetry/{scenario}.json"


def test_execution_options_validate_run_metadata() -> None:
    options = HarnessExecutionOptions(frames=1, dt=0.5, run_index=1)
    with pytest.raises(PrototypeHarnessError):
        options.validate()

    options = HarnessExecutionOptions(frames=1, dt=0.5, run_index=0, run_count=1)
    with pytest.raises(PrototypeHarnessError):
        options.validate()

    options = HarnessExecutionOptions(frames=1, dt=0.5, run_index=2, run_count=1)
    with pytest.raises(PrototypeHarnessError):
        options.validate()

    options = HarnessExecutionOptions(frames=1, dt=math.nan)
    with pytest.raises(PrototypeHarnessError):
        options.validate()

    options = HarnessExecutionOptions(frames=1, dt=math.inf)
    with pytest.raises(PrototypeHarnessError):
        options.validate()


def test_describe_configuration_returns_metadata(tmp_path: Path) -> None:
    config_path = _write_configuration(tmp_path)
    configuration = load_configuration(config_path)

    harness = PrototypeHarness(
        configuration,
        asset_search_paths=[tmp_path],
        project_root=tmp_path,
        config_directory=tmp_path,
    )
    description = harness.describe_configuration()
    description_payload = configuration_summary_to_dict(description)

    assert description.selected_dataset == "remesh-sample"
    assert description.datasets[0].label == "remesh-sample"
    assert description.datasets[0].schema_version == 2
    assert description.datasets[0].feature_preservation["lock_feature_edges"] is True
    assert description.datasets[0].parameterization is not None
    assert description.datasets[0].parameterization["chart_count"] == 1
    assert pytest.approx(description.datasets[0].statistics["splits"], rel=1e-6) == 1.0
    assert pytest.approx(description.datasets[0].statistics["collapses"], rel=1e-6) == 1.0
    assert pytest.approx(description.datasets[0].statistics["duration_ms"], rel=1e-6) == 1.0
    assert pytest.approx(description.datasets[0].statistics["triangles"], rel=1e-6) == 2.0
    assert description.datasets[0].statistics["triangle_quality"]["min"] == pytest.approx(0.8)
    assert description.datasets[0].statistics["triangle_quality"]["mean"] == pytest.approx(0.9)
    assert description.datasets[0].statistics["triangle_quality"]["max"] == pytest.approx(0.95)
    assert description.runtime.dataset == "remesh-sample"
    assert description.rendering is not None
    assert description.rendering.preset == "research-baseline"
    assert description.rendering.schema_version == 1
    assert description.runtime.schema_version == 2
    assert description.telemetry is not None
    assert description.telemetry.schema_version == 2
    assert len(description.telemetry.outputs) == 2
    expected_output_path = (tmp_path / "telemetry" / "remesh-sample.json").resolve()
    assert description.telemetry.outputs[0].path == str(expected_output_path)
    assert description.telemetry.outputs[0].template == "telemetry/{scenario}.json"
    assert description.telemetry.metrics[1].statistic == "p95"
    assert description.benchmarks is not None
    assert description.benchmarks.schema_version == 1
    assert len(description.benchmarks.scenarios) == 1
    scenario = description.benchmarks.scenarios[0]
    assert scenario.identifier == "remesh-baseline"
    assert scenario.engine.command == ("python", "run_engine.py", "--output", "{output_path}")
    assert scenario.metrics[0].threshold.limit == pytest.approx(0.05)
    assert description_payload["datasets"][0]["label"] == "remesh-sample"
    assert description_payload["runtime"]["hot_reload"] == {
        "enabled": False,
        "watch_interval_seconds": 0.5,
    }
    assert description_payload["datasets"][0]["id"] == "remesh-sample"
    assert description_payload["datasets"][0]["kind"] == "geometry.remesh"
    assert description_payload["datasets"][0]["schema_version"] == 2
    stats_payload = description_payload["datasets"][0]["statistics"]
    assert pytest.approx(stats_payload["splits"], rel=1e-6) == 1.0
    assert pytest.approx(stats_payload["collapses"], rel=1e-6) == 1.0
    assert pytest.approx(stats_payload["duration_ms"], rel=1e-6) == 1.0
    assert pytest.approx(stats_payload["triangles"], rel=1e-6) == 2.0
    assert pytest.approx(stats_payload["triangle_quality"]["min"], rel=1e-6) == 0.8
    assert pytest.approx(stats_payload["triangle_quality"]["mean"], rel=1e-6) == 0.9
    assert pytest.approx(stats_payload["triangle_quality"]["max"], rel=1e-6) == 0.95
    assert description_payload["telemetry"]["schema_version"] == 2
    telemetry_output_payload = description_payload["telemetry"]["outputs"][0]
    assert telemetry_output_payload["kind"] == "file"
    assert telemetry_output_payload["path"] == str(expected_output_path)
    assert telemetry_output_payload["template"] == "telemetry/{scenario}.json"
    assert description_payload["benchmarks"]["schema_version"] == 1
    assert (
        description_payload["benchmarks"]["scenarios"][0]["engine"]["command"][0]
        == "python"
    )
    assert description.datasets[0].assets
    assert all(status.verified for status in description.datasets[0].assets)
    asset_payloads = description_payload["datasets"][0]["assets"]
    assert asset_payloads and asset_payloads[0]["verified"] is True


def test_describe_selected_dataset_returns_summary(tmp_path: Path) -> None:
    config_path = _write_configuration(tmp_path)
    configuration = load_configuration(config_path)

    harness = PrototypeHarness(
        configuration,
        asset_search_paths=[tmp_path],
        project_root=tmp_path,
        config_directory=tmp_path,
    )
    summary = harness.describe_selected_dataset()
    assert summary is not None
    assert summary.identifier == "remesh-sample"
    assert summary.assets
    assert all(status.verified for status in summary.assets)


def test_prototype_harness_missing_dataset_raises(tmp_path: Path) -> None:
    config_path = _write_configuration(tmp_path)
    text = json.loads(config_path.read_text(encoding="utf-8"))
    text["runtime"]["dataset"] = "unknown"
    config_path.write_text(json.dumps(text), encoding="utf-8")

    with pytest.raises(ConfigurationSchemaError):
        load_configuration(config_path)


def test_prototype_harness_detects_missing_assets(tmp_path: Path) -> None:
    config_path = _write_configuration(tmp_path)
    missing_asset = tmp_path / "assets" / "input.obj"
    missing_asset.unlink()
    configuration = load_configuration(config_path)

    with pytest.raises(PrototypeHarnessError) as excinfo:
        PrototypeHarness(
            configuration,
            asset_search_paths=[tmp_path],
            project_root=tmp_path,
            config_directory=tmp_path,
        )

    assert "remesh-sample" in str(excinfo.value)
    assert "asset" in str(excinfo.value)


def test_prototype_harness_verifies_assets_without_explicit_paths(tmp_path: Path) -> None:
    config_path = _write_configuration(tmp_path)
    configuration = load_configuration(config_path)

    harness = PrototypeHarness(
        configuration,
        project_root=tmp_path,
        config_directory=tmp_path,
    )

    dataset_summary = harness.describe_selected_dataset()
    assert dataset_summary is not None
    assert dataset_summary.assets
    assert all(status.verified for status in dataset_summary.assets)


def test_prototype_harness_validates_scene_manifest(tmp_path: Path) -> None:
    config_path = _write_configuration(tmp_path)
    payload = json.loads(config_path.read_text(encoding="utf-8"))
    scene_dir = tmp_path / "scenes"
    scene_dir.mkdir(parents=True, exist_ok=True)
    scene_manifest = scene_dir / "sample.scene"
    scene_manifest.write_text("scene {}", encoding="utf-8")
    payload["runtime"]["scene"] = {
        "manifest": scene_manifest.relative_to(tmp_path).as_posix(),
        "entry_point": "main",
    }
    config_path.write_text(json.dumps(payload), encoding="utf-8")

    configuration = load_configuration(config_path)
    harness = PrototypeHarness(
        configuration,
        asset_search_paths=[tmp_path],
        project_root=tmp_path,
        config_directory=tmp_path,
    )

    description = harness.describe_configuration()
    assert description.runtime.scene_manifest_path == str(scene_manifest.resolve())
    runtime_payload = configuration_summary_to_dict(description)["runtime"]
    assert runtime_payload["scene_manifest_path"] == str(scene_manifest.resolve())


def test_prototype_harness_missing_scene_manifest_raises(tmp_path: Path) -> None:
    config_path = _write_configuration(tmp_path)
    payload = json.loads(config_path.read_text(encoding="utf-8"))
    payload["runtime"]["scene"] = {
        "manifest": "scenes/missing.scene",
        "entry_point": "main",
    }
    config_path.write_text(json.dumps(payload), encoding="utf-8")
    configuration = load_configuration(config_path)

    with pytest.raises(PrototypeHarnessError) as excinfo:
        PrototypeHarness(
            configuration,
            asset_search_paths=[tmp_path],
            project_root=tmp_path,
            config_directory=tmp_path,
        )

    message = str(excinfo.value)
    assert "scene manifest" in message
    assert "missing.scene" in message


def test_run_headless_unknown_telemetry_placeholder(tmp_path: Path) -> None:
    config_path = _write_configuration(tmp_path)
    payload = json.loads(config_path.read_text(encoding="utf-8"))
    payload["telemetry"]["outputs"][0]["path"] = "telemetry/{unknown}.json"
    config_path.write_text(json.dumps(payload), encoding="utf-8")
    configuration = load_configuration(config_path)

    harness = PrototypeHarness(
        configuration,
        asset_search_paths=[tmp_path],
        project_root=tmp_path,
        config_directory=tmp_path,
    )

    with pytest.raises(PrototypeHarnessError) as excinfo:
        harness.run_headless(HarnessExecutionOptions(dry_run=True))

    assert "placeholder" in str(excinfo.value)


def test_load_harness_validates_sections(tmp_path: Path) -> None:
    config_path = _write_configuration(tmp_path)
    harness = load_harness(config_path)
    assert harness.selected_dataset is not None


def test_load_harness_requires_runtime_section(tmp_path: Path) -> None:
    config_path = _write_configuration(tmp_path)
    text = json.loads(config_path.read_text(encoding="utf-8"))
    del text["runtime"]
    config_path.write_text(json.dumps(text), encoding="utf-8")

    with pytest.raises(PrototypeHarnessError):
        load_harness(config_path)


def test_load_harness_respects_require_schema(tmp_path: Path) -> None:
    config_path = _write_configuration(tmp_path)
    payload = json.loads(config_path.read_text(encoding="utf-8"))
    del payload["datasets"][0]["schema"]
    config_path.write_text(json.dumps(payload), encoding="utf-8")

    # Legacy manifests still load when the flag is disabled.
    harness = load_harness(config_path)
    assert harness.selected_dataset is not None

    # Enforcing schema headers should now fail.
    with pytest.raises(PrototypeHarnessError):
        load_harness(config_path, require_schema=True)


def test_summarize_formats_output() -> None:
    summary = HarnessRunSummary(
        dataset_id="remesh-sample",
        scenario_label="remesh-baseline",
        rendering_preset="research-baseline",
        shading_mode="deferred",
        frames_executed=10,
        timestep_seconds=0.016,
        average_tick_ms=0.75,
        dispatch_order=("geometry::remesh",),
        dispatch_durations_ms=(1.5,),
    )
    assert (
        summarize(summary)
        == (
            "scenario=remesh-baseline dataset=remesh-sample preset=research-baseline shading=deferred "
            "frames=10 dt=0.016000 avg_ms=0.750000 dispatches=1"
        )
    )

    summary_payload = run_summary_to_dict(summary)
    assert summary_payload["dataset"] == "remesh-sample"
    assert summary_payload["scenario"] == "remesh-baseline"
    assert summary_payload["frames"] == 10
    assert summary_payload["dispatch_order"] == ["geometry::remesh"]
    assert summary_payload["dispatch_durations_ms"] == [1.5]
    assert summary_payload["telemetry_outputs"] == []


def test_summarize_includes_run_metadata() -> None:
    summary = HarnessRunSummary(
        dataset_id="remesh-sample",
        scenario_label=None,
        rendering_preset="research-baseline",
        shading_mode="deferred",
        frames_executed=5,
        timestep_seconds=0.02,
        run_index=2,
        run_count=3,
    )
    text = summarize(summary)
    assert "run=2/3" in text

    summary_payload = run_summary_to_dict(summary)
    assert summary_payload["scenario"] is None
    assert summary_payload["run_index"] == 2
    assert summary_payload["run_count"] == 3
    assert summary_payload["telemetry_outputs"] == []


def test_cli_exports_json(tmp_path: Path, capsys: pytest.CaptureFixture[str]) -> None:
    config_path = _write_configuration(tmp_path)
    describe_path = tmp_path / "exports" / "describe.json"
    summary_path = tmp_path / "exports" / "summary.json"

    from scripts.prototyping import run_prototype_harness

    exit_code = run_prototype_harness.main(
        [
            "--config",
            str(config_path),
            "--dry-run",
            "--describe-json",
            str(describe_path),
            "--summary-json",
            str(summary_path),
        ]
    )

    captured = capsys.readouterr()
    assert exit_code == 0
    assert describe_path.exists()
    assert summary_path.exists()

    assert "Dataset assets (remesh-sample): 2 total, 0 failures" in captured.out
    assert "- source_mesh" in captured.out
    assert "- output_mesh" in captured.out

    description_payload = json.loads(describe_path.read_text(encoding="utf-8"))
    summary_payload = json.loads(summary_path.read_text(encoding="utf-8"))

    assert description_payload["selected_dataset"] == "remesh-sample"
    assert description_payload["rendering"]["preset"] == "research-baseline"
    assert description_payload["benchmarks"]["scenarios"][0]["id"] == "remesh-baseline"
    assert summary_payload["frames"] == 0
    assert summary_payload["average_tick_ms"] is None
    assert summary_payload["scenario"] is None
    assert summary_payload["dispatch_order"] == []
    assert summary_payload["dispatch_durations_ms"] == []
    assert len(summary_payload["telemetry_outputs"]) == 2
    telemetry_outputs = summary_payload["telemetry_outputs"]
    expected_output_path = (tmp_path / "telemetry" / "remesh-sample.json").resolve()
    assert telemetry_outputs[0] == {
        "kind": "file",
        "path": str(expected_output_path),
        "template": "telemetry/{scenario}.json",
    }
    assert telemetry_outputs[1] == {"kind": "stdout"}
    assert "Configuration:" in captured.out


def test_cli_repeat_generates_multiple_summaries(
    tmp_path: Path, capsys: pytest.CaptureFixture[str]
) -> None:
    config_path = _write_configuration(tmp_path)
    base_summary = tmp_path / "summary.json"

    from scripts.prototyping import run_prototype_harness

    exit_code = run_prototype_harness.main(
        [
            "--config",
            str(config_path),
            "--dry-run",
            "--summary-json",
            str(base_summary),
            "--repeat",
            "3",
        ]
    )

    captured = capsys.readouterr()
    assert exit_code == 0
    assert "Dataset assets (remesh-sample): 2 total, 0 failures" in captured.out
    for index in range(1, 4):
        suffix = f"summary-run{index:02d}.json"
        path = base_summary.with_name(suffix)
        assert path.exists()
        payload = json.loads(path.read_text(encoding="utf-8"))
        assert payload["run_index"] == index
        assert payload["run_count"] == 3
        assert payload["scenario"] is None
        outputs = payload["telemetry_outputs"]
        assert len(outputs) == 2
        assert outputs[0]["template"] == "telemetry/{scenario}.json"
        assert outputs[0]["path"].endswith("telemetry/remesh-sample.json")
    assert "Dry run summary [1/3]" in captured.out
    assert "Dry run summary [3/3]" in captured.out


def test_cli_list_benchmarks(tmp_path: Path, capsys: pytest.CaptureFixture[str]) -> None:
    config_path = _write_configuration(tmp_path)

    from scripts.prototyping import run_prototype_harness

    exit_code = run_prototype_harness.main(
        ["--config", str(config_path), "--dry-run", "--list-benchmarks"]
    )

    captured = capsys.readouterr()
    assert exit_code == 0
    assert "Benchmark scenarios:" in captured.out
    assert "remesh-baseline" in captured.out


def test_cli_dry_run(monkeypatch: pytest.MonkeyPatch, tmp_path: Path, capsys: pytest.CaptureFixture[str]) -> None:
    config_path = _write_configuration(tmp_path)

    def _load(
        path: str,
        *,
        runtime_factory: Callable[[], object] | None = None,
        require_schema: bool | None = None,
    ) -> PrototypeHarness:
        config_path = Path(path)
        configuration = load_configuration(config_path, require_schema=require_schema)
        factory = runtime_factory or (lambda: _MockRuntime(ticks=[]))
        return PrototypeHarness(
            configuration,
            runtime_factory=factory,
            asset_search_paths=[config_path.parent],
            project_root=config_path.parent,
            config_directory=config_path.parent,
        )

    from scripts.prototyping import run_prototype_harness

    monkeypatch.setattr("scripts.prototyping.run_prototype_harness.load_harness", _load)

    exit_code = run_prototype_harness.main(["--config", str(config_path), "--dry-run"])
    captured = capsys.readouterr()
    assert exit_code == 0
    assert "Dry run summary" in captured.out


def test_cli_require_schema_flag(tmp_path: Path, capsys: pytest.CaptureFixture[str]) -> None:
    config_path = _write_configuration(tmp_path)
    payload = json.loads(config_path.read_text(encoding="utf-8"))
    del payload["datasets"][0]["schema"]
    config_path.write_text(json.dumps(payload), encoding="utf-8")

    from scripts.prototyping import run_prototype_harness

    exit_code = run_prototype_harness.main(
        ["--config", str(config_path), "--dry-run", "--require-schema"]
    )
    captured = capsys.readouterr()
    assert exit_code == 2
    assert "schema" in captured.err
    assert captured.out == ""


def test_cli_case_study_support(tmp_path: Path, capsys: pytest.CaptureFixture[str]) -> None:
    summary_path = tmp_path / "summary.json"
    summary_path_rendering = tmp_path / "summary_rendering.json"

    from scripts.prototyping import run_prototype_harness

    exit_code = run_prototype_harness.main(
        [
            "--case-study",
            "geometry-baseline",
            "--dry-run",
            "--summary-json",
            str(summary_path),
        ]
    )
    captured = capsys.readouterr()

    assert exit_code == 0
    assert summary_path.exists()

    summary_payload = json.loads(summary_path.read_text(encoding="utf-8"))

    assert summary_payload["dataset"] == "geometry-remesh-baseline"
    assert summary_payload["average_tick_ms"] is None
    assert summary_payload["scenario"] == "geometry-baseline"
    telemetry_outputs = summary_payload["telemetry_outputs"]
    assert telemetry_outputs, "expected telemetry outputs"
    assert telemetry_outputs[0]["path"].endswith(
        "telemetry/geometry-baseline/geometry-baseline.json"
    )
    assert "Selected case study 'geometry-baseline'" in captured.out
    assert "Dry run summary" in captured.out
    assert "scenario=geometry-baseline" in captured.out

    exit_code_rendering = run_prototype_harness.main(
        [
            "--case-study",
            "rendering-debug",
            "--dry-run",
            "--summary-json",
            str(summary_path_rendering),
        ]
    )
    captured_rendering = capsys.readouterr()

    assert exit_code_rendering == 0
    assert summary_path_rendering.exists()

    summary_rendering = json.loads(summary_path_rendering.read_text(encoding="utf-8"))

    assert summary_rendering["dataset"] == "rendering-light-volume"
    assert summary_rendering["average_tick_ms"] is None
    assert summary_rendering["scenario"] == "rendering-debug"
    rendering_outputs = summary_rendering["telemetry_outputs"]
    assert rendering_outputs, "expected telemetry outputs"
    assert rendering_outputs[0]["path"].endswith(
        "telemetry/rendering-debug/rendering-debug.json"
    )
    assert "Selected case study 'rendering-debug'" in captured_rendering.out
    assert "scenario=rendering-debug" in captured_rendering.out


def test_cli_lists_case_studies(tmp_path: Path, capsys: pytest.CaptureFixture[str]) -> None:
    json_path = tmp_path / "case_studies.json"

    from scripts.prototyping import run_prototype_harness

    exit_code = run_prototype_harness.main(
        ["--list-case-studies", "--case-studies-json", str(json_path)]
    )
    captured = capsys.readouterr()

    assert exit_code == 0
    assert "Available case studies" in captured.out
    assert json_path.exists()

    payload = json.loads(json_path.read_text(encoding="utf-8"))
    assert "case_studies" in payload
    assert isinstance(payload["case_studies"], list)
    assert payload["case_studies"], "expected registered case studies"


def test_cli_integration_with_sample_assets(
    tmp_path: Path, capsys: pytest.CaptureFixture[str]
) -> None:
    project_root = Path(__file__).resolve().parents[2]
    manifest_path = project_root / "assets" / "datasets" / "remesh_sample" / "manifest.json"
    manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
    datasets = manifest.get("datasets")
    assert isinstance(datasets, list) and datasets, "sample dataset manifest is required"
    dataset_id = datasets[0]["id"]

    configuration = {
        "datasets": datasets,
        "rendering": {
            "schema": {"id": "ai-004.rendering", "version": 1},
            "preset": "research-baseline",
            "options": {
                "shading_mode": "deferred",
                "resolution": {"width": 1280, "height": 720},
            },
        },
        "runtime": {
            "schema": {"id": "ai-004.runtime", "version": 1},
            "dataset": dataset_id,
            "simulation": {"timestep_seconds": 1.0 / 60.0, "max_substeps": 2},
        },
        "telemetry": {
            "schema": {"id": "ai-004.telemetry", "version": 1},
            "outputs": [{"type": "file", "path": "telemetry/{scenario}.json"}],
        },
    }
    config_path = tmp_path / "ai004_config.json"
    config_path.write_text(json.dumps(configuration), encoding="utf-8")

    from scripts.prototyping import run_prototype_harness

    exit_code = run_prototype_harness.main(
        ["--config", str(config_path), "--dry-run", "--require-schema"]
    )
    captured = capsys.readouterr()

    assert exit_code == 0
    assert "Configuration:" in captured.out
    assert f"dataset={dataset_id}" in captured.out
    assert "Dry run summary" in captured.out
    assert captured.err == ""


def test_docs_sample_configuration_is_loadable() -> None:
    sample_config = Path(__file__).resolve().parents[2] / "docs" / "examples" / "ai004_sample.json"
    assert sample_config.exists(), "docs/examples/ai004_sample.json is required for onboarding"

    harness = load_harness(sample_config, require_schema=True)
    assert harness.selected_dataset is not None
    assert harness.configuration.rendering is not None
    assert harness.configuration.runtime is not None

