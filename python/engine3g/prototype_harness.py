"""Prototype harness implementation for AI-004 runtime workflows."""

from __future__ import annotations

import hashlib
import math
import os
import re
from dataclasses import dataclass
from pathlib import Path
from typing import Callable, Dict, Iterable, Mapping, Optional, Sequence, Tuple

from .config_schema import (
    Ai004Configuration,
    BenchmarkCommandConfig,
    BenchmarkConfig,
    BenchmarkMetricConfig,
    BenchmarkScenarioConfig,
    ConfigurationSchemaError,
    DatasetEntry,
    DatasetManifest,
    RenderingConfig,
    RuntimeConfig,
    TelemetryConfig,
    TelemetryOutputConfig,
)
from .loader import EngineRuntimeHandle, EngineLibraryNotFound, load_runtime

__all__ = [
    "BenchmarkCommandSummary",
    "BenchmarkMetricSummary",
    "BenchmarkMetricThresholdSummary",
    "BenchmarkScenarioSummary",
    "HarnessExecutionOptions",
    "HarnessRunSummary",
    "DatasetAssetStatus",
    "DatasetSummary",
    "HarnessConfigurationSummary",
    "HarnessBenchmarkSummary",
    "HarnessTelemetrySummary",
    "PrototypeHarness",
    "PrototypeHarnessError",
    "TelemetryMetricSummary",
    "TelemetryOutputSummary",
    "TelemetrySamplingSummary",
    "RenderingSummary",
    "RuntimeSummary",
    "configuration_summary_to_dict",
    "load_harness",
    "run_summary_to_dict",
    "summarize",
]


class PrototypeHarnessError(RuntimeError):
    """Raised when harness configuration cannot be resolved."""


class _StrictSubstitutions(dict[str, object]):
    """Mapping that raises on missing template substitutions."""

    def __missing__(self, key: str) -> str:  # pragma: no cover - defensive guard
        raise PrototypeHarnessError(f"missing substitution for placeholder '{key}'")


@dataclass(frozen=True)
class DatasetAssetStatus:
    """Integrity summary for an asset declared within a dataset entry."""

    role: str
    path: str
    resolved_path: str
    exists: bool
    expected_size_bytes: Optional[int]
    actual_size_bytes: Optional[int]
    expected_sha256: Optional[str]
    actual_sha256: Optional[str]
    verified: bool
    message: Optional[str] = None

    def to_dict(self) -> Dict[str, object]:
        payload: Dict[str, object] = {
            "role": self.role,
            "path": self.path,
            "resolved_path": self.resolved_path,
            "exists": self.exists,
            "expected_size_bytes": self.expected_size_bytes,
            "actual_size_bytes": self.actual_size_bytes,
            "expected_sha256": self.expected_sha256,
            "actual_sha256": self.actual_sha256,
            "verified": self.verified,
        }
        if self.message is not None:
            payload["message"] = self.message
        return payload


@dataclass(frozen=True)
class HarnessExecutionOptions:
    """Execution parameters controlling headless harness runs."""

    frames: int = 600
    dt: float = 1.0 / 60.0
    dry_run: bool = False
    run_index: Optional[int] = None
    run_count: Optional[int] = None
    scenario_label: Optional[str] = None

    def validate(self) -> None:
        if self.frames <= 0:
            raise PrototypeHarnessError("frames must be greater than zero")
        if not math.isfinite(self.dt) or self.dt <= 0.0:
            raise PrototypeHarnessError("dt must be a positive, finite number")
        if self.run_index is not None and self.run_index <= 0:
            raise PrototypeHarnessError("run_index must be greater than zero when provided")
        if self.run_count is not None and self.run_count <= 0:
            raise PrototypeHarnessError("run_count must be greater than zero when provided")
        if self.run_index is not None and self.run_count is None:
            raise PrototypeHarnessError("run_index requires run_count to be specified")
        if self.run_index is not None and self.run_index > self.run_count:
            raise PrototypeHarnessError("run_index cannot exceed run_count")
        if self.scenario_label is not None and not self.scenario_label:
            raise PrototypeHarnessError("scenario_label must be a non-empty string when provided")


@dataclass(frozen=True)
class HarnessRunSummary:
    """Summary of a completed harness execution."""

    dataset_id: Optional[str]
    scenario_label: Optional[str]
    rendering_preset: Optional[str]
    shading_mode: Optional[str]
    frames_executed: int
    timestep_seconds: float
    average_tick_ms: Optional[float] = None
    dispatch_order: Tuple[str, ...] = ()
    dispatch_durations_ms: Tuple[float, ...] = ()
    telemetry_outputs: Tuple[TelemetryOutputSummary, ...] = ()
    run_index: Optional[int] = None
    run_count: Optional[int] = None


RuntimeFactory = Callable[[], EngineRuntimeHandle]


@dataclass(frozen=True)
class DatasetSummary:
    """Aggregated dataset metadata for UI surfaces."""

    identifier: str
    label: Optional[str]
    kind: str
    schema_id: str
    schema_version: int
    tags: Tuple[str, ...]
    source_generator: str
    source_mesh: str
    source_mesh_sha256: Optional[str]
    source_mesh_size_bytes: Optional[int]
    output_mesh: str
    output_mesh_sha256: Optional[str]
    output_mesh_size_bytes: Optional[int]
    remeshing_mode: str
    remeshing_targets: Optional[Dict[str, float]]
    feature_preservation: Dict[str, object]
    parameterization: Optional[Dict[str, object]]
    statistics: Dict[str, object]
    metrics: Dict[str, Dict[str, float]]
    assets: Tuple[DatasetAssetStatus, ...]

    def to_dict(self) -> Dict[str, object]:
        payload: Dict[str, object] = {
            "id": self.identifier,
            "label": self.label,
            "kind": self.kind,
            "schema_id": self.schema_id,
            "schema_version": self.schema_version,
            "tags": list(self.tags),
            "source_generator": self.source_generator,
            "source_mesh": self.source_mesh,
            "output_mesh": self.output_mesh,
            "remeshing_mode": self.remeshing_mode,
            "feature_preservation": dict(self.feature_preservation),
            "statistics": dict(self.statistics),
            "metrics": {key: dict(value) for key, value in self.metrics.items()},
            "assets": [asset.to_dict() for asset in self.assets],
        }
        if self.source_mesh_sha256 is not None:
            payload["source_mesh_sha256"] = self.source_mesh_sha256
        if self.source_mesh_size_bytes is not None:
            payload["source_mesh_size_bytes"] = self.source_mesh_size_bytes
        if self.output_mesh_sha256 is not None:
            payload["output_mesh_sha256"] = self.output_mesh_sha256
        if self.output_mesh_size_bytes is not None:
            payload["output_mesh_size_bytes"] = self.output_mesh_size_bytes
        if self.remeshing_targets:
            payload["remeshing_targets"] = dict(self.remeshing_targets)
        if self.parameterization:
            payload["parameterization"] = dict(self.parameterization)
        return payload


@dataclass(frozen=True)
class RenderingSummary:
    """Rendering preset details used by the sandbox UI."""

    preset: str
    shading_mode: str
    resolution: Tuple[int, int]
    schema_version: int
    overlays: Dict[str, bool]

    def to_dict(self) -> Dict[str, object]:
        return {
            "preset": self.preset,
            "shading_mode": self.shading_mode,
            "resolution": {"width": self.resolution[0], "height": self.resolution[1]},
            "schema_version": self.schema_version,
            "overlays": dict(self.overlays),
        }


@dataclass(frozen=True)
class RuntimeSummary:
    """Runtime configuration excerpt for sandbox integration."""

    dataset: Optional[str]
    scene_manifest: Optional[str]
    scene_entry_point: Optional[str]
    camera: Optional[Dict[str, object]]
    simulation: Optional[Dict[str, object]]
    schema_version: int
    hot_reload: Dict[str, object]
    scene_manifest_path: Optional[str] = None

    def to_dict(self) -> Dict[str, object]:
        payload: Dict[str, object] = {
            "dataset": self.dataset,
            "scene_manifest": self.scene_manifest,
            "scene_entry_point": self.scene_entry_point,
            "schema_version": self.schema_version,
            "hot_reload": dict(self.hot_reload),
        }
        if self.camera is not None:
            payload["camera"] = dict(self.camera)
        if self.simulation is not None:
            payload["simulation"] = dict(self.simulation)
        if self.scene_manifest_path is not None:
            payload["scene_manifest_path"] = self.scene_manifest_path
        return payload


@dataclass(frozen=True)
class TelemetryOutputSummary:
    """Telemetry output destination emitted by the harness."""

    kind: str
    path: Optional[str]
    template: Optional[str] = None

    def to_dict(self) -> Dict[str, object]:
        payload: Dict[str, object] = {"kind": self.kind}
        if self.path is not None:
            payload["path"] = self.path
        if self.template is not None:
            payload["template"] = self.template
        return payload


@dataclass(frozen=True)
class TelemetryMetricSummary:
    """Telemetry metric configuration surfaced to the sandbox."""

    name: str
    statistic: str

    def to_dict(self) -> Dict[str, object]:
        return {"name": self.name, "statistic": self.statistic}


@dataclass(frozen=True)
class BenchmarkMetricThresholdSummary:
    """Regression threshold metadata for benchmark metrics."""

    mode: str
    limit: float

    def to_dict(self) -> Dict[str, object]:
        return {"mode": self.mode, "limit": self.limit}


@dataclass(frozen=True)
class BenchmarkMetricSummary:
    """Benchmark metric descriptor exposed to tooling."""

    name: str
    higher_is_better: bool
    threshold: BenchmarkMetricThresholdSummary

    def to_dict(self) -> Dict[str, object]:
        return {
            "name": self.name,
            "higher_is_better": self.higher_is_better,
            "threshold": self.threshold.to_dict(),
        }


@dataclass(frozen=True)
class BenchmarkCommandSummary:
    """Execution details for a benchmark scenario command."""

    command: Optional[Tuple[str, ...]]
    output: str

    def to_dict(self) -> Dict[str, object]:
        payload: Dict[str, object] = {"output": self.output}
        if self.command is not None:
            payload["command"] = list(self.command)
        return payload


@dataclass(frozen=True)
class BenchmarkScenarioSummary:
    """Aggregated benchmark scenario metadata for UI integrations."""

    identifier: str
    name: str
    dataset: Optional[str]
    rendering_preset: Optional[str]
    runtime_profile: Optional[str]
    engine: BenchmarkCommandSummary
    reference: BenchmarkCommandSummary
    metrics: Tuple[BenchmarkMetricSummary, ...]

    def to_dict(self) -> Dict[str, object]:
        return {
            "id": self.identifier,
            "name": self.name,
            "dataset": self.dataset,
            "rendering_preset": self.rendering_preset,
            "runtime_profile": self.runtime_profile,
            "engine": self.engine.to_dict(),
            "reference": self.reference.to_dict(),
            "metrics": [metric.to_dict() for metric in self.metrics],
        }


@dataclass(frozen=True)
class HarnessBenchmarkSummary:
    """Benchmark configuration summary surfaced by the harness."""

    schema_version: int
    scenarios: Tuple[BenchmarkScenarioSummary, ...]

    def to_dict(self) -> Dict[str, object]:
        return {
            "schema_version": self.schema_version,
            "scenarios": [scenario.to_dict() for scenario in self.scenarios],
        }


@dataclass(frozen=True)
class TelemetrySamplingSummary:
    """Telemetry sampling cadence used by the harness."""

    frame_interval: int
    include_debug_overlays: bool

    def to_dict(self) -> Dict[str, object]:
        return {
            "frame_interval": self.frame_interval,
            "include_debug_overlays": self.include_debug_overlays,
        }


@dataclass(frozen=True)
class HarnessTelemetrySummary:
    """Aggregated telemetry configuration for UI integrations."""

    schema_version: int
    outputs: Tuple[TelemetryOutputSummary, ...]
    metrics: Tuple[TelemetryMetricSummary, ...]
    sampling: Optional[TelemetrySamplingSummary]

    def to_dict(self) -> Dict[str, object]:
        payload: Dict[str, object] = {
            "schema_version": self.schema_version,
            "outputs": [output.to_dict() for output in self.outputs],
            "metrics": [metric.to_dict() for metric in self.metrics],
        }
        if self.sampling is not None:
            payload["sampling"] = self.sampling.to_dict()
        return payload


@dataclass(frozen=True)
class HarnessConfigurationSummary:
    """Human-readable description of the harness configuration."""

    datasets: Tuple[DatasetSummary, ...]
    selected_dataset: Optional[str]
    rendering: Optional[RenderingSummary]
    runtime: RuntimeSummary
    telemetry: Optional[HarnessTelemetrySummary]
    benchmarks: Optional[HarnessBenchmarkSummary]

    def to_dict(self) -> Dict[str, object]:
        return {
            "datasets": [dataset.to_dict() for dataset in self.datasets],
            "selected_dataset": self.selected_dataset,
            "rendering": self.rendering.to_dict() if self.rendering else None,
            "runtime": self.runtime.to_dict(),
            "telemetry": self.telemetry.to_dict() if self.telemetry else None,
            "benchmarks": self.benchmarks.to_dict() if self.benchmarks else None,
        }


class PrototypeHarness:
    """Headless harness that validates AI-004 configurations and executes runtime ticks."""

    _PLACEHOLDER_SAFE_PATTERN = re.compile(r"[^a-z0-9_-]+")

    def __init__(
        self,
        configuration: Ai004Configuration,
        *,
        runtime_factory: RuntimeFactory | None = None,
        asset_search_paths: Sequence[Path | str] | None = None,
        project_root: Path | None = None,
        config_directory: Path | None = None,
    ) -> None:
        self._configuration = configuration
        self._runtime_factory = runtime_factory or load_runtime
        self._asset_search_paths = self._normalise_asset_search_paths(asset_search_paths)
        self._project_root = project_root.resolve() if project_root is not None else None
        self._config_directory = config_directory.resolve() if config_directory is not None else None
        self._selected_dataset = self._resolve_dataset(configuration.datasets, configuration.runtime)
        self._dataset_assets: Dict[str, Tuple[DatasetAssetStatus, ...]] = {}
        self._runtime_scene_manifest: Optional[DatasetAssetStatus] = None
        should_verify_assets = self._should_verify_assets()
        if should_verify_assets:
            for entry in configuration.datasets.datasets:
                self._dataset_assets[entry.identifier] = self._verify_dataset_assets(entry)
        if (
            should_verify_assets
            and configuration.runtime is not None
            and configuration.runtime.scene_manifest is not None
        ):
            self._runtime_scene_manifest = self._verify_scene_manifest(configuration.runtime.scene_manifest)

    @property
    def configuration(self) -> Ai004Configuration:
        """Return the validated AI-004 configuration."""

        return self._configuration

    @property
    def selected_dataset(self) -> Optional[DatasetEntry]:
        """Return the dataset entry referenced by the runtime configuration, if any."""

        return self._selected_dataset

    @staticmethod
    def _resolve_dataset(
        manifest: DatasetManifest,
        runtime_config: Optional[RuntimeConfig],
    ) -> Optional[DatasetEntry]:
        if runtime_config is None or runtime_config.dataset is None:
            return None

        slug = runtime_config.dataset
        for entry in manifest.datasets:
            if entry.identifier == slug:
                return entry
        raise PrototypeHarnessError(
            f"runtime.dataset references unknown dataset '{slug}'. Provide a matching entry in datasets[].",
        )

    def _rendering_config(self) -> Optional[RenderingConfig]:
        return self._configuration.rendering

    def describe_configuration(self) -> HarnessConfigurationSummary:
        """Return metadata describing datasets and runtime presets for UI layers."""

        datasets = tuple(self._describe_dataset(entry) for entry in self._configuration.datasets.datasets)
        rendering_config = self._rendering_config()
        rendering_summary = self._describe_rendering(rendering_config) if rendering_config else None
        runtime_summary = self._describe_runtime(self._configuration.runtime)
        telemetry_summary = (
            self._describe_telemetry(self._configuration.telemetry)
            if self._configuration.telemetry
            else None
        )
        benchmark_summary = (
            self._describe_benchmarks(self._configuration.benchmarks)
            if self._configuration.benchmarks
            else None
        )
        return HarnessConfigurationSummary(
            datasets=datasets,
            selected_dataset=self._selected_dataset.identifier if self._selected_dataset else None,
            rendering=rendering_summary,
            runtime=runtime_summary,
            telemetry=telemetry_summary,
            benchmarks=benchmark_summary,
        )

    def describe_selected_dataset(self) -> Optional[DatasetSummary]:
        """Return a summary for the dataset referenced by the runtime configuration."""

        if self._selected_dataset is None:
            return None
        return self._describe_dataset(self._selected_dataset)


    def _telemetry_outputs(
        self,
        *,
        scenario_override: Optional[str] = None,
        execution: Optional[HarnessExecutionOptions] = None,
    ) -> Tuple[TelemetryOutputSummary, ...]:
        """Return telemetry outputs declared in the configuration."""

        if self._configuration.telemetry is None:
            return ()
        return tuple(
            self._summarize_telemetry_output(
                output,
                scenario_override=scenario_override,
                execution=execution,
            )
            for output in self._configuration.telemetry.outputs
        )

    def _telemetry_substitutions(
        self,
        *,
        scenario_override: Optional[str] = None,
        execution: Optional[HarnessExecutionOptions] = None,
    ) -> Mapping[str, object]:
        dataset_id = self._selected_dataset.identifier if self._selected_dataset else None
        rendering = self._rendering_config()
        scenario_value = scenario_override or dataset_id or "default"
        dataset_token = self._sanitize_placeholder_token(dataset_id or "default")
        scenario_token = self._sanitize_placeholder_token(scenario_value)
        preset_token = self._sanitize_placeholder_token(rendering.preset) if rendering else "default"
        shading_token = (
            self._sanitize_placeholder_token(rendering.shading_mode)
            if rendering and rendering.shading_mode is not None
            else "default"
        )
        defaults: Dict[str, object] = {
            "dataset": dataset_token,
            "scenario": scenario_token,
            "rendering_preset": preset_token,
            "shading_mode": shading_token,
        }
        if execution is not None:
            defaults["frames"] = execution.frames
            defaults["dt"] = execution.dt
            if execution.run_index is not None:
                defaults["run_index"] = execution.run_index
            if execution.run_count is not None:
                defaults["run_count"] = execution.run_count
        if self._config_directory is not None:
            defaults["config_dir"] = str(self._config_directory)
        if self._project_root is not None:
            defaults["project_root"] = str(self._project_root)
        return defaults

    @classmethod
    def _sanitize_placeholder_token(cls, value: str) -> str:
        """Return a filesystem-safe token for telemetry placeholders."""

        normalized = value.strip().lower()
        if not normalized:
            return "default"

        separators = {"/", "\\"}
        if os.sep:
            separators.add(os.sep)
        if os.altsep:
            separators.add(os.altsep)

        sanitized_chars: list[str] = []
        for char in normalized:
            if char in separators or char.isspace():
                sanitized_chars.append("-")
            elif char.isalnum() or char in {"-", "_"}:
                sanitized_chars.append(char)
            else:
                sanitized_chars.append("-")

        sanitized = "".join(sanitized_chars)
        sanitized = cls._PLACEHOLDER_SAFE_PATTERN.sub("-", sanitized)
        sanitized = re.sub(r"-+", "-", sanitized).strip("-")
        return sanitized or "default"

    def _resolve_output_path(
        self,
        template: Optional[str],
        *,
        scenario_override: Optional[str] = None,
        execution: Optional[HarnessExecutionOptions] = None,
    ) -> Tuple[Optional[str], Optional[str]]:
        if template is None:
            return None, None
        substitutions = self._telemetry_substitutions(
            scenario_override=scenario_override,
            execution=execution,
        )
        try:
            formatted = template.format_map(_StrictSubstitutions(substitutions))
        except KeyError as error:  # pragma: no cover - defensive guard
            raise PrototypeHarnessError(f"missing substitution for placeholder '{error.args[0]}'") from error
        except ValueError as error:
            raise PrototypeHarnessError(f"invalid telemetry output template '{template}': {error}") from error
        path = Path(formatted)
        if not path.is_absolute():
            base = self._config_directory or Path.cwd()
            path = (base / path).resolve()
        resolved = str(path)
        template_value = template if template != resolved else None
        return resolved, template_value

    def _summarize_telemetry_output(
        self,
        output: TelemetryOutputConfig,
        *,
        scenario_override: Optional[str] = None,
        execution: Optional[HarnessExecutionOptions] = None,
    ) -> TelemetryOutputSummary:
        resolved_path, template = self._resolve_output_path(
            output.path,
            scenario_override=scenario_override,
            execution=execution,
        )
        return TelemetryOutputSummary(kind=output.kind, path=resolved_path, template=template)

    def run_headless(self, options: HarnessExecutionOptions | None = None) -> HarnessRunSummary:
        """Execute a fixed-timestep runtime loop and return a summary."""

        execution = options or HarnessExecutionOptions()
        execution.validate()

        rendering = self._rendering_config()
        average_tick_ms: Optional[float] = None
        telemetry_outputs = self._telemetry_outputs(
            scenario_override=execution.scenario_label,
            execution=execution,
        )

        if execution.dry_run:
            return HarnessRunSummary(
                dataset_id=self._selected_dataset.identifier if self._selected_dataset else None,
                rendering_preset=rendering.preset if rendering else None,
                shading_mode=rendering.shading_mode if rendering else None,
                frames_executed=0,
                timestep_seconds=execution.dt,
                average_tick_ms=None,
                dispatch_order=(),
                dispatch_durations_ms=(),
                telemetry_outputs=telemetry_outputs,
                run_index=execution.run_index,
                run_count=execution.run_count,
                scenario_label=execution.scenario_label,
            )

        try:
            runtime = self._runtime_factory()
        except EngineLibraryNotFound as error:  # pragma: no cover - depends on local environment
            raise PrototypeHarnessError(str(error)) from error

        frames_executed = 0
        dispatch_order: Tuple[str, ...] = ()
        dispatch_durations_ms: Tuple[float, ...] = ()
        with runtime:
            for _ in range(execution.frames):
                runtime.tick(execution.dt)
                frames_executed += 1
            average_tick_ms = runtime.average_tick_ms()
            order = tuple(runtime.dispatch_order())
            durations = tuple(runtime.dispatch_durations())
            if len(order) != len(durations):
                count = min(len(order), len(durations))
                order = order[:count]
                durations = durations[:count]
            dispatch_order = order
            dispatch_durations_ms = tuple(duration * 1000.0 for duration in durations)

        return HarnessRunSummary(
            dataset_id=self._selected_dataset.identifier if self._selected_dataset else None,
            rendering_preset=rendering.preset if rendering else None,
            shading_mode=rendering.shading_mode if rendering else None,
            frames_executed=frames_executed,
            timestep_seconds=execution.dt,
            average_tick_ms=average_tick_ms,
            dispatch_order=dispatch_order,
            dispatch_durations_ms=dispatch_durations_ms,
            telemetry_outputs=telemetry_outputs,
            run_index=execution.run_index,
            run_count=execution.run_count,
            scenario_label=execution.scenario_label,
        )

    def _describe_dataset(self, entry: DatasetEntry) -> DatasetSummary:
        assets = self._dataset_assets.get(entry.identifier, tuple())

        remeshing_targets: Optional[Dict[str, float]] = None
        if entry.remeshing_targets is not None:
            remeshing_targets = {
                key: value
                for key, value in vars(entry.remeshing_targets).items()
                if value is not None
            }

        parameterization: Optional[Dict[str, float]] = None
        if entry.parameterization is not None:
            parameterization = {
                "mode": entry.parameterization.mode,
                "texel_density": entry.parameterization.texel_density,
                "chart_count": entry.parameterization.chart_count,
                "average_stretch": entry.parameterization.average_stretch,
                "max_stretch": entry.parameterization.max_stretch,
                "fill_ratio": entry.parameterization.fill_ratio,
                "total_seam_length": entry.parameterization.total_seam_length,
            }
            if entry.parameterization.target_texel_density is not None:
                parameterization["target_texel_density"] = entry.parameterization.target_texel_density
            if entry.parameterization.atlas_area is not None:
                parameterization["atlas_area"] = entry.parameterization.atlas_area
            if entry.parameterization.total_chart_area is not None:
                parameterization["total_chart_area"] = entry.parameterization.total_chart_area
            if entry.parameterization.charts:
                parameterization["charts"] = [
                    {
                        "index": chart.index,
                        "min_uv": list(chart.min_uv),
                        "max_uv": list(chart.max_uv),
                        "translation": list(chart.translation),
                        "scale": chart.scale,
                        "area": chart.area,
                        "boundary_length": chart.boundary_length,
                    }
                    for chart in entry.parameterization.charts
                ]

        statistics: Dict[str, object] = {
            "iterations": float(entry.statistics.iteration_count),
            "max_error": entry.statistics.max_error,
            "min_edge_length": entry.statistics.min_edge_length,
            "max_edge_length": entry.statistics.max_edge_length,
            "max_surface_deviation": entry.statistics.max_surface_deviation,
            "mean_surface_deviation": entry.statistics.mean_surface_deviation,
            "rms_surface_deviation": entry.statistics.rms_surface_deviation,
        }
        if entry.statistics.split_count is not None:
            statistics["splits"] = float(entry.statistics.split_count)
        if entry.statistics.collapse_count is not None:
            statistics["collapses"] = float(entry.statistics.collapse_count)
        if entry.statistics.duration_ms is not None:
            statistics["duration_ms"] = entry.statistics.duration_ms
        if entry.statistics.triangle_count is not None:
            statistics["triangles"] = float(entry.statistics.triangle_count)
        if entry.statistics.triangle_quality is not None:
            statistics["triangle_quality"] = {
                "min": entry.statistics.triangle_quality.minimum,
                "mean": entry.statistics.triangle_quality.mean,
                "max": entry.statistics.triangle_quality.maximum,
            }

        metrics: Dict[str, Dict[str, float]] = {
            "input": {
                "vertices": float(entry.input_metrics.vertices),
                "faces": float(entry.input_metrics.faces),
                "edge_length_min": entry.input_metrics.edge_length.minimum,
                "edge_length_max": entry.input_metrics.edge_length.maximum,
                "edge_length_mean": entry.input_metrics.edge_length.mean,
            },
            "output": {
                "vertices": float(entry.output_metrics.vertices),
                "faces": float(entry.output_metrics.faces),
                "edge_length_min": entry.output_metrics.edge_length.minimum,
                "edge_length_max": entry.output_metrics.edge_length.maximum,
                "edge_length_mean": entry.output_metrics.edge_length.mean,
            },
        }

        label = entry.job_label or entry.identifier

        feature_preservation: Dict[str, object] = {
            "lock_boundary_edges": entry.feature_preservation.lock_boundary_edges,
            "lock_feature_edges": entry.feature_preservation.lock_feature_edges,
            "minimum_feature_angle_degrees": entry.feature_preservation.minimum_feature_angle_degrees,
        }

        return DatasetSummary(
            identifier=entry.identifier,
            label=label,
            kind=entry.kind,
            schema_id=entry.schema_id,
            schema_version=entry.schema_version,
            tags=entry.tags,
            source_generator=entry.source_generator,
            source_mesh=entry.source_mesh,
            source_mesh_sha256=entry.source_mesh_sha256,
            source_mesh_size_bytes=entry.source_mesh_size_bytes,
            output_mesh=entry.output_mesh,
            output_mesh_sha256=entry.output_mesh_sha256,
            output_mesh_size_bytes=entry.output_mesh_size_bytes,
            remeshing_mode=entry.remeshing_mode,
            remeshing_targets=remeshing_targets,
            feature_preservation=feature_preservation,
            parameterization=parameterization,
            statistics=statistics,
            metrics=metrics,
            assets=assets,
        )

    @staticmethod
    def _describe_rendering(rendering: RenderingConfig) -> RenderingSummary:
        overlays = {
            "normals": rendering.overlay_normals,
            "uv": rendering.overlay_uv,
            "material": rendering.overlay_material,
            "light_volume": rendering.overlay_light_volume,
        }
        return RenderingSummary(
            preset=rendering.preset,
            shading_mode=rendering.shading_mode,
            resolution=(rendering.width, rendering.height),
            schema_version=rendering.schema_version,
            overlays=overlays,
        )

    def _describe_runtime(self, runtime: RuntimeConfig) -> RuntimeSummary:
        camera: Optional[Dict[str, object]] = None
        if runtime.camera is not None:
            camera = {
                "mode": runtime.camera.mode,
                "position": runtime.camera.position,
                "target": runtime.camera.target,
            }

        simulation: Optional[Dict[str, object]] = None
        if runtime.simulation is not None:
            simulation = {
                "timestep_seconds": runtime.simulation.timestep_seconds,
                "max_substeps": runtime.simulation.max_substeps,
            }

        hot_reload: Dict[str, object] = {
            "enabled": runtime.hot_reload.enabled,
        }
        if runtime.hot_reload.watch_interval_seconds is not None:
            hot_reload["watch_interval_seconds"] = runtime.hot_reload.watch_interval_seconds

        resolved_scene_manifest: Optional[str] = None
        if self._runtime_scene_manifest is not None and runtime.scene_manifest is not None:
            resolved_scene_manifest = self._runtime_scene_manifest.resolved_path

        return RuntimeSummary(
            dataset=runtime.dataset,
            scene_manifest=runtime.scene_manifest,
            scene_entry_point=runtime.scene_entry_point,
            camera=camera,
            simulation=simulation,
            schema_version=runtime.schema_version,
            hot_reload=hot_reload,
            scene_manifest_path=resolved_scene_manifest,
        )

    def _describe_telemetry(self, telemetry: TelemetryConfig) -> HarnessTelemetrySummary:
        outputs = tuple(self._summarize_telemetry_output(output) for output in telemetry.outputs)
        metrics = tuple(
            TelemetryMetricSummary(name=metric.name, statistic=metric.statistic)
            for metric in telemetry.metrics
        )
        sampling = None
        if telemetry.sampling is not None:
            sampling = TelemetrySamplingSummary(
                frame_interval=telemetry.sampling.frame_interval,
                include_debug_overlays=telemetry.sampling.include_debug_overlays,
            )
        return HarnessTelemetrySummary(
            schema_version=telemetry.schema_version,
            outputs=outputs,
            metrics=metrics,
            sampling=sampling,
        )

    @staticmethod
    def _describe_benchmark_command(command: BenchmarkCommandConfig) -> BenchmarkCommandSummary:
        return BenchmarkCommandSummary(command=command.command, output=command.output)

    @classmethod
    def _describe_benchmark_metric(
        cls, metric: BenchmarkMetricConfig
    ) -> BenchmarkMetricSummary:
        threshold = BenchmarkMetricThresholdSummary(mode=metric.threshold.mode, limit=metric.threshold.limit)
        return BenchmarkMetricSummary(
            name=metric.name,
            higher_is_better=metric.higher_is_better,
            threshold=threshold,
        )

    @classmethod
    def _describe_benchmark_scenario(
        cls, scenario: BenchmarkScenarioConfig
    ) -> BenchmarkScenarioSummary:
        metrics = tuple(cls._describe_benchmark_metric(metric) for metric in scenario.metrics)
        return BenchmarkScenarioSummary(
            identifier=scenario.identifier,
            name=scenario.name,
            dataset=scenario.dataset,
            rendering_preset=scenario.rendering_preset,
            runtime_profile=scenario.runtime_profile,
            engine=cls._describe_benchmark_command(scenario.engine),
            reference=cls._describe_benchmark_command(scenario.reference),
            metrics=metrics,
        )

    @classmethod
    def _describe_benchmarks(cls, benchmarks: BenchmarkConfig) -> HarnessBenchmarkSummary:
        scenarios = tuple(cls._describe_benchmark_scenario(scenario) for scenario in benchmarks.scenarios)
        return HarnessBenchmarkSummary(schema_version=benchmarks.schema_version, scenarios=scenarios)


    @staticmethod
    def _normalise_asset_search_paths(paths: Sequence[Path | str] | None) -> Tuple[Path, ...]:
        if not paths:
            return tuple()
        normalised: Dict[Path, None] = {}
        for entry in paths:
            resolved = Path(entry).resolve()
            normalised.setdefault(resolved, None)
        return tuple(normalised.keys())

    def _should_verify_assets(self) -> bool:
        return (
            bool(self._asset_search_paths)
            or self._config_directory is not None
            or self._project_root is not None
        )

    @staticmethod
    def _compute_sha256(path: Path) -> str:
        digest = hashlib.sha256()
        with path.open("rb") as stream:
            for chunk in iter(lambda: stream.read(65536), b""):
                digest.update(chunk)
        return digest.hexdigest()

    def _candidate_asset_paths(self, declared_path: str) -> Tuple[Path, ...]:
        declared = Path(declared_path)
        if declared.is_absolute():
            return (declared,)
        candidates: Iterable[Path] = ()
        if self._asset_search_paths:
            candidates = ((base / declared).resolve() for base in self._asset_search_paths)
        resolved: Dict[Path, None] = {}
        for candidate in candidates:
            resolved.setdefault(candidate, None)
        if self._config_directory is not None:
            resolved.setdefault((self._config_directory / declared).resolve(), None)
        if self._project_root is not None:
            resolved.setdefault((self._project_root / declared).resolve(), None)
        if self._project_root is not None:
            dataset_root = (self._project_root / "assets" / "datasets").resolve()
            if dataset_root.exists():
                if len(declared.parts) > 1:
                    candidate = (dataset_root / declared).resolve()
                    resolved.setdefault(candidate, None)
                else:
                    for match in dataset_root.rglob(declared.name):
                        resolved.setdefault(match.resolve(), None)
        fallback = (Path.cwd() / declared).resolve()
        resolved.setdefault(fallback, None)
        return tuple(resolved.keys())

    def _build_asset_status(
        self,
        *,
        role: str,
        declared_path: str,
        expected_sha256: Optional[str],
        expected_size: Optional[int],
    ) -> DatasetAssetStatus:
        candidates = self._candidate_asset_paths(declared_path)
        resolved_path = candidates[0] if candidates else Path(declared_path).resolve()
        actual_path: Optional[Path] = None
        for candidate in candidates:
            if candidate.exists():
                actual_path = candidate
                break
        if actual_path is None:
            message = "asset not found"
            if candidates:
                message = f"asset not found (searched: {', '.join(str(candidate) for candidate in candidates)})"
            return DatasetAssetStatus(
                role=role,
                path=declared_path,
                resolved_path=str(resolved_path),
                exists=False,
                expected_size_bytes=expected_size,
                actual_size_bytes=None,
                expected_sha256=expected_sha256,
                actual_sha256=None,
                verified=False,
                message=message,
            )

        if not actual_path.is_file():
            return DatasetAssetStatus(
                role=role,
                path=declared_path,
                resolved_path=str(actual_path),
                exists=False,
                expected_size_bytes=expected_size,
                actual_size_bytes=None,
                expected_sha256=expected_sha256,
                actual_sha256=None,
                verified=False,
                message="resolved path is not a file",
            )

        try:
            actual_size = int(actual_path.stat().st_size)
        except OSError as error:
            return DatasetAssetStatus(
                role=role,
                path=declared_path,
                resolved_path=str(actual_path),
                exists=False,
                expected_size_bytes=expected_size,
                actual_size_bytes=None,
                expected_sha256=expected_sha256,
                actual_sha256=None,
                verified=False,
                message=f"unable to stat asset: {error}",
            )

        actual_sha256: Optional[str] = None
        try:
            actual_sha256 = self._compute_sha256(actual_path)
        except OSError as error:
            return DatasetAssetStatus(
                role=role,
                path=declared_path,
                resolved_path=str(actual_path),
                exists=True,
                expected_size_bytes=expected_size,
                actual_size_bytes=actual_size,
                expected_sha256=expected_sha256,
                actual_sha256=None,
                verified=False,
                message=f"unable to read asset: {error}",
            )

        verified = True
        message_parts: list[str] = []
        if expected_size is not None and actual_size != expected_size:
            verified = False
            message_parts.append(
                f"size mismatch (expected {expected_size}, found {actual_size})"
            )
        if expected_sha256 is not None and actual_sha256 != expected_sha256:
            verified = False
            message_parts.append("sha256 mismatch")
        message = ", ".join(message_parts) if message_parts else None

        return DatasetAssetStatus(
            role=role,
            path=declared_path,
            resolved_path=str(actual_path),
            exists=True,
            expected_size_bytes=expected_size,
            actual_size_bytes=actual_size,
            expected_sha256=expected_sha256,
            actual_sha256=actual_sha256,
            verified=verified,
            message=message,
        )

    def _verify_dataset_assets(self, entry: DatasetEntry) -> Tuple[DatasetAssetStatus, ...]:
        statuses = (
            self._build_asset_status(
                role="source_mesh",
                declared_path=entry.source_mesh,
                expected_sha256=entry.source_mesh_sha256,
                expected_size=entry.source_mesh_size_bytes,
            ),
            self._build_asset_status(
                role="output_mesh",
                declared_path=entry.output_mesh,
                expected_sha256=entry.output_mesh_sha256,
                expected_size=entry.output_mesh_size_bytes,
            ),
        )
        failures = [status for status in statuses if not status.verified]
        if failures:
            details = "; ".join(
                f"{status.role}: {status.message or 'verification failed'} (path={status.resolved_path})"
                for status in failures
            )
            raise PrototypeHarnessError(
                f"dataset '{entry.identifier}' assets failed verification: {details}"
            )
        return statuses

    def _verify_scene_manifest(self, declared_path: str) -> DatasetAssetStatus:
        status = self._build_asset_status(
            role="scene_manifest",
            declared_path=declared_path,
            expected_sha256=None,
            expected_size=None,
        )
        if not status.exists:
            details = status.message or "asset not found"
            raise PrototypeHarnessError(
                "runtime scene manifest '{}' not found: {}".format(declared_path, details)
            )
        return status


def load_harness(
    path: str,
    *,
    runtime_factory: RuntimeFactory | None = None,
    require_schema: bool | None = None,
) -> PrototypeHarness:
    """Load an AI-004 configuration from *path* and construct a harness."""

    from .config_schema import load_configuration  # Local import to avoid cycle during module init

    config_path = Path(path)
    try:
        configuration = load_configuration(config_path, require_schema=require_schema)
    except ConfigurationSchemaError as error:
        raise PrototypeHarnessError(str(error)) from error
    if configuration.runtime is None:
        raise PrototypeHarnessError("configuration.runtime section is required for harness execution")
    if not configuration.datasets.datasets:
        raise PrototypeHarnessError("configuration must provide at least one dataset entry")
    asset_paths = [config_path.parent.resolve()]
    project_root = _discover_project_root(config_path.parent.resolve())
    if project_root is None:
        project_root = Path(__file__).resolve().parents[2]
    if project_root is not None:
        asset_paths.append(project_root)
    return PrototypeHarness(
        configuration,
        runtime_factory=runtime_factory,
        asset_search_paths=asset_paths,
        project_root=project_root,
        config_directory=config_path.parent.resolve(),
    )


def summarize(summary: HarnessRunSummary) -> str:
    """Render a human-readable summary line for CLI output."""

    dataset = summary.dataset_id or "<none>"
    scenario = f"scenario={summary.scenario_label} " if summary.scenario_label else ""
    preset = summary.rendering_preset or "<unspecified>"
    shading = summary.shading_mode or "<unspecified>"
    average = (
        f" avg_ms={summary.average_tick_ms:.6f}" if summary.average_tick_ms is not None else ""
    )
    dispatch = (
        f" dispatches={len(summary.dispatch_order)}" if summary.dispatch_order else ""
    )
    run = ""
    if summary.run_index is not None and summary.run_count is not None:
        run = f" run={summary.run_index}/{summary.run_count}"
    return (
        f"{scenario}dataset={dataset} preset={preset} shading={shading} "
        f"frames={summary.frames_executed} dt={summary.timestep_seconds:.6f}{average}{run}{dispatch}"
    )


def configuration_summary_to_dict(summary: HarnessConfigurationSummary) -> Dict[str, object]:
    """Convert a configuration summary to a JSON-serialisable dictionary."""

    return summary.to_dict()


def run_summary_to_dict(summary: HarnessRunSummary) -> Dict[str, object]:
    """Convert a run summary to a JSON-serialisable dictionary."""

    payload: Dict[str, object] = {
        "dataset": summary.dataset_id,
        "scenario": summary.scenario_label,
        "rendering_preset": summary.rendering_preset,
        "shading_mode": summary.shading_mode,
        "frames": summary.frames_executed,
        "timestep_seconds": summary.timestep_seconds,
        "average_tick_ms": summary.average_tick_ms,
        "dispatch_order": list(summary.dispatch_order),
        "dispatch_durations_ms": list(summary.dispatch_durations_ms),
        "telemetry_outputs": [output.to_dict() for output in summary.telemetry_outputs],
    }
    if summary.run_index is not None:
        payload["run_index"] = summary.run_index
    if summary.run_count is not None:
        payload["run_count"] = summary.run_count
    return payload


def _discover_project_root(start: Path) -> Optional[Path]:
    for candidate in (start, *start.parents):
        if (candidate / ".git").exists() or (candidate / "CMakeLists.txt").exists():
            return candidate
    return None

