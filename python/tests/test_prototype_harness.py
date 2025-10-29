from __future__ import annotations

import json
import sys
from dataclasses import dataclass
from pathlib import Path
from typing import Callable, List

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


def _write_configuration(tmp_path: Path) -> Path:
    config = {
        "datasets": [
            {
                "schema": {"id": "ai-004.dataset", "version": 2},
                "id": "remesh-sample",
                "kind": "geometry.remesh",
                "tags": ["geometry", "remesh"],
                "source": {
                    "generator": "geometry_remesh",
                    "mesh": "assets/input.obj",
                    "mesh_sha256": "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef",
                    "mesh_size_bytes": 128,
                },
                "outputs": {
                    "mesh": "assets/output.obj",
                    "mesh_sha256": "fedcba9876543210fedcba9876543210fedcba9876543210fedcba9876543210",
                    "mesh_size_bytes": 256,
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
    runtime = _MockRuntime(ticks=[], average_tick_value=1.5)

    harness = PrototypeHarness(configuration, runtime_factory=lambda: runtime)
    summary = harness.run_headless(HarnessExecutionOptions(frames=3, dt=0.5))

    assert runtime.ticks == [0.5, 0.5, 0.5]
    assert runtime.shutdown_count == 1
    assert summary.frames_executed == 3
    assert summary.dataset_id == "remesh-sample"
    assert summary.rendering_preset == "research-baseline"
    assert summary.average_tick_ms == pytest.approx(1.5)


def test_describe_configuration_returns_metadata(tmp_path: Path) -> None:
    config_path = _write_configuration(tmp_path)
    configuration = load_configuration(config_path)

    harness = PrototypeHarness(configuration)
    description = harness.describe_configuration()
    description_payload = configuration_summary_to_dict(description)

    assert description.selected_dataset == "remesh-sample"
    assert description.datasets[0].label == "remesh-sample"
    assert description.datasets[0].schema_version == 2
    assert description.datasets[0].feature_preservation["lock_feature_edges"] is True
    assert description.datasets[0].parameterization is not None
    assert description.datasets[0].parameterization["chart_count"] == 1
    assert description.runtime.dataset == "remesh-sample"
    assert description.rendering is not None
    assert description.rendering.preset == "research-baseline"
    assert description.rendering.schema_version == 1
    assert description.runtime.schema_version == 2
    assert description.telemetry is not None
    assert description.telemetry.schema_version == 2
    assert len(description.telemetry.outputs) == 2
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
    assert description_payload["telemetry"]["schema_version"] == 2
    assert description_payload["telemetry"]["outputs"][0]["kind"] == "file"
    assert description_payload["benchmarks"]["schema_version"] == 1
    assert (
        description_payload["benchmarks"]["scenarios"][0]["engine"]["command"][0]
        == "python"
    )


def test_prototype_harness_missing_dataset_raises(tmp_path: Path) -> None:
    config_path = _write_configuration(tmp_path)
    text = json.loads(config_path.read_text(encoding="utf-8"))
    text["runtime"]["dataset"] = "unknown"
    config_path.write_text(json.dumps(text), encoding="utf-8")

    with pytest.raises(ConfigurationSchemaError):
        load_configuration(config_path)


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
        rendering_preset="research-baseline",
        shading_mode="deferred",
        frames_executed=10,
        timestep_seconds=0.016,
        average_tick_ms=0.75,
    )
    assert (
        summarize(summary)
        == (
            "dataset=remesh-sample preset=research-baseline shading=deferred "
            "frames=10 dt=0.016000 avg_ms=0.750000"
        )
    )

    summary_payload = run_summary_to_dict(summary)
    assert summary_payload["dataset"] == "remesh-sample"
    assert summary_payload["frames"] == 10


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

    description_payload = json.loads(describe_path.read_text(encoding="utf-8"))
    summary_payload = json.loads(summary_path.read_text(encoding="utf-8"))

    assert description_payload["selected_dataset"] == "remesh-sample"
    assert description_payload["rendering"]["preset"] == "research-baseline"
    assert description_payload["benchmarks"]["scenarios"][0]["id"] == "remesh-baseline"
    assert summary_payload["frames"] == 0
    assert summary_payload["average_tick_ms"] is None
    assert "Configuration:" in captured.out


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
        configuration = load_configuration(path, require_schema=require_schema)
        return PrototypeHarness(configuration, runtime_factory=lambda: _MockRuntime(ticks=[]))

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
    assert "Selected case study 'geometry-baseline'" in captured.out
    assert "Dry run summary" in captured.out

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
    assert "Selected case study 'rendering-debug'" in captured_rendering.out


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

