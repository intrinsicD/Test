"""Prototype harness implementation for AI-004 runtime workflows."""

from __future__ import annotations

from dataclasses import dataclass
from typing import Callable, Dict, Optional, Tuple

from .config_schema import (
    Ai004Configuration,
    ConfigurationSchemaError,
    DatasetEntry,
    DatasetManifest,
    RenderingConfig,
    RuntimeConfig,
    TelemetryConfig,
)
from .loader import EngineRuntimeHandle, EngineLibraryNotFound, load_runtime

__all__ = [
    "HarnessExecutionOptions",
    "HarnessRunSummary",
    "DatasetSummary",
    "HarnessConfigurationSummary",
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


@dataclass(frozen=True)
class HarnessExecutionOptions:
    """Execution parameters controlling headless harness runs."""

    frames: int = 600
    dt: float = 1.0 / 60.0
    dry_run: bool = False

    def validate(self) -> None:
        if self.frames <= 0:
            raise PrototypeHarnessError("frames must be greater than zero")
        if not float(self.dt) or self.dt <= 0.0:
            raise PrototypeHarnessError("dt must be a positive number")


@dataclass(frozen=True)
class HarnessRunSummary:
    """Summary of a completed harness execution."""

    dataset_id: Optional[str]
    rendering_preset: Optional[str]
    shading_mode: Optional[str]
    frames_executed: int
    timestep_seconds: float
    average_tick_ms: Optional[float] = None


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
    statistics: Dict[str, float]
    metrics: Dict[str, Dict[str, float]]

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
        return payload


@dataclass(frozen=True)
class TelemetryOutputSummary:
    """Telemetry output destination emitted by the harness."""

    kind: str
    path: Optional[str]

    def to_dict(self) -> Dict[str, object]:
        payload: Dict[str, object] = {"kind": self.kind}
        if self.path is not None:
            payload["path"] = self.path
        return payload


@dataclass(frozen=True)
class TelemetryMetricSummary:
    """Telemetry metric configuration surfaced to the sandbox."""

    name: str
    statistic: str

    def to_dict(self) -> Dict[str, object]:
        return {"name": self.name, "statistic": self.statistic}


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

    def to_dict(self) -> Dict[str, object]:
        return {
            "datasets": [dataset.to_dict() for dataset in self.datasets],
            "selected_dataset": self.selected_dataset,
            "rendering": self.rendering.to_dict() if self.rendering else None,
            "runtime": self.runtime.to_dict(),
            "telemetry": self.telemetry.to_dict() if self.telemetry else None,
        }


class PrototypeHarness:
    """Headless harness that validates AI-004 configurations and executes runtime ticks."""

    def __init__(
        self,
        configuration: Ai004Configuration,
        *,
        runtime_factory: RuntimeFactory | None = None,
    ) -> None:
        self._configuration = configuration
        self._runtime_factory = runtime_factory or load_runtime
        self._selected_dataset = self._resolve_dataset(configuration.datasets, configuration.runtime)

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
        return HarnessConfigurationSummary(
            datasets=datasets,
            selected_dataset=self._selected_dataset.identifier if self._selected_dataset else None,
            rendering=rendering_summary,
            runtime=runtime_summary,
            telemetry=telemetry_summary,
        )

    def run_headless(self, options: HarnessExecutionOptions | None = None) -> HarnessRunSummary:
        """Execute a fixed-timestep runtime loop and return a summary."""

        execution = options or HarnessExecutionOptions()
        execution.validate()

        rendering = self._rendering_config()
        average_tick_ms: Optional[float] = None

        if execution.dry_run:
            return HarnessRunSummary(
                dataset_id=self._selected_dataset.identifier if self._selected_dataset else None,
                rendering_preset=rendering.preset if rendering else None,
                shading_mode=rendering.shading_mode if rendering else None,
                frames_executed=0,
                timestep_seconds=execution.dt,
                average_tick_ms=None,
            )

        try:
            runtime = self._runtime_factory()
        except EngineLibraryNotFound as error:  # pragma: no cover - depends on local environment
            raise PrototypeHarnessError(str(error)) from error

        frames_executed = 0
        with runtime:
            for _ in range(execution.frames):
                runtime.tick(execution.dt)
                frames_executed += 1
            average_tick_ms = runtime.average_tick_ms()

        return HarnessRunSummary(
            dataset_id=self._selected_dataset.identifier if self._selected_dataset else None,
            rendering_preset=rendering.preset if rendering else None,
            shading_mode=rendering.shading_mode if rendering else None,
            frames_executed=frames_executed,
            timestep_seconds=execution.dt,
            average_tick_ms=average_tick_ms,
        )

    @staticmethod
    def _describe_dataset(entry: DatasetEntry) -> DatasetSummary:
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

        statistics: Dict[str, float] = {
            "iterations": float(entry.statistics.iteration_count),
            "max_error": entry.statistics.max_error,
            "min_edge_length": entry.statistics.min_edge_length,
            "max_edge_length": entry.statistics.max_edge_length,
            "max_surface_deviation": entry.statistics.max_surface_deviation,
            "mean_surface_deviation": entry.statistics.mean_surface_deviation,
            "rms_surface_deviation": entry.statistics.rms_surface_deviation,
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

    @staticmethod
    def _describe_runtime(runtime: RuntimeConfig) -> RuntimeSummary:
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

        return RuntimeSummary(
            dataset=runtime.dataset,
            scene_manifest=runtime.scene_manifest,
            scene_entry_point=runtime.scene_entry_point,
            camera=camera,
            simulation=simulation,
            schema_version=runtime.schema_version,
            hot_reload=hot_reload,
        )

    @staticmethod
    def _describe_telemetry(telemetry: TelemetryConfig) -> HarnessTelemetrySummary:
        outputs = tuple(
            TelemetryOutputSummary(kind=output.kind, path=output.path)
            for output in telemetry.outputs
        )
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


def load_harness(
    path: str,
    *,
    runtime_factory: RuntimeFactory | None = None,
    require_schema: bool | None = None,
) -> PrototypeHarness:
    """Load an AI-004 configuration from *path* and construct a harness."""

    from .config_schema import load_configuration  # Local import to avoid cycle during module init

    try:
        configuration = load_configuration(path, require_schema=require_schema)
    except ConfigurationSchemaError as error:
        raise PrototypeHarnessError(str(error)) from error
    if configuration.runtime is None:
        raise PrototypeHarnessError("configuration.runtime section is required for harness execution")
    if not configuration.datasets.datasets:
        raise PrototypeHarnessError("configuration must provide at least one dataset entry")
    return PrototypeHarness(configuration, runtime_factory=runtime_factory)


def summarize(summary: HarnessRunSummary) -> str:
    """Render a human-readable summary line for CLI output."""

    dataset = summary.dataset_id or "<none>"
    preset = summary.rendering_preset or "<unspecified>"
    shading = summary.shading_mode or "<unspecified>"
    average = (
        f" avg_ms={summary.average_tick_ms:.6f}" if summary.average_tick_ms is not None else ""
    )
    return (
        f"dataset={dataset} preset={preset} shading={shading} "
        f"frames={summary.frames_executed} dt={summary.timestep_seconds:.6f}{average}"
    )


def configuration_summary_to_dict(summary: HarnessConfigurationSummary) -> Dict[str, object]:
    """Convert a configuration summary to a JSON-serialisable dictionary."""

    return summary.to_dict()


def run_summary_to_dict(summary: HarnessRunSummary) -> Dict[str, object]:
    """Convert a run summary to a JSON-serialisable dictionary."""

    return {
        "dataset": summary.dataset_id,
        "rendering_preset": summary.rendering_preset,
        "shading_mode": summary.shading_mode,
        "frames": summary.frames_executed,
        "timestep_seconds": summary.timestep_seconds,
        "average_tick_ms": summary.average_tick_ms,
    }

