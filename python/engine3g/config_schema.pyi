from __future__ import annotations

import os
from dataclasses import dataclass
from typing import Optional, Tuple, Union


class ConfigurationSchemaError(RuntimeError): ...


@dataclass
class RemeshingTargets:
    target_edge_length: Optional[float]
    relative_edge_scale: Optional[float]
    max_normal_deviation_degrees: Optional[float]
    max_surface_deviation: Optional[float]


@dataclass
class FeaturePreservation:
    lock_boundary_edges: bool
    lock_feature_edges: bool
    minimum_feature_angle_degrees: float


@dataclass
class EdgeLengthMetrics:
    minimum: float
    maximum: float
    mean: float


@dataclass
class MeshMetrics:
    vertices: int
    faces: int
    edge_length: EdgeLengthMetrics


@dataclass
class ParameterizationChart:
    index: int
    min_uv: Tuple[float, float]
    max_uv: Tuple[float, float]
    translation: Tuple[float, float]
    scale: float
    area: float
    boundary_length: float


@dataclass
class ParameterizationSummary:
    mode: str
    target_texel_density: Optional[float]
    texel_density: float
    chart_count: int
    average_stretch: float
    max_stretch: float
    fill_ratio: float
    total_seam_length: float
    atlas_area: Optional[float]
    total_chart_area: Optional[float]
    charts: Tuple[ParameterizationChart, ...]


@dataclass
class DatasetStatistics:
    iteration_count: int
    max_error: float
    min_edge_length: float
    max_edge_length: float
    max_surface_deviation: float
    mean_surface_deviation: float
    rms_surface_deviation: float


@dataclass
class DatasetEntry:
    identifier: str
    schema_id: str
    schema_version: int
    kind: str
    tags: Tuple[str, ...]
    source_generator: str
    source_mesh: str
    source_mesh_sha256: Optional[str]
    source_mesh_size_bytes: Optional[int]
    output_mesh: str
    output_mesh_sha256: Optional[str]
    output_mesh_size_bytes: Optional[int]
    remeshing_mode: str
    remeshing_targets: Optional[RemeshingTargets]
    feature_preservation: FeaturePreservation
    input_metrics: MeshMetrics
    output_metrics: MeshMetrics
    parameterization: Optional[ParameterizationSummary]
    statistics: DatasetStatistics
    job_label: Optional[str]


@dataclass
class DatasetManifest:
    datasets: Tuple[DatasetEntry, ...]


@dataclass
class RenderingConfig:
    schema_version: int
    preset: str
    shading_mode: str
    width: int
    height: int
    overlay_normals: bool
    overlay_uv: bool
    overlay_material: bool
    overlay_light_volume: bool


@dataclass
class RuntimeCameraConfig:
    mode: str
    position: Optional[Tuple[float, float, float]]
    target: Optional[Tuple[float, float, float]]


@dataclass
class RuntimeSimulationConfig:
    timestep_seconds: float
    max_substeps: int


@dataclass
class RuntimeHotReloadConfig:
    enabled: bool
    watch_interval_seconds: Optional[float]


@dataclass
class RuntimeConfig:
    schema_version: int
    dataset: Optional[str]
    scene_manifest: Optional[str]
    scene_entry_point: Optional[str]
    camera: Optional[RuntimeCameraConfig]
    simulation: Optional[RuntimeSimulationConfig]
    hot_reload: RuntimeHotReloadConfig


@dataclass
class BenchmarkThreshold:
    mode: str
    limit: float


@dataclass
class BenchmarkMetricConfig:
    name: str
    higher_is_better: bool
    threshold: BenchmarkThreshold


@dataclass
class BenchmarkCommandConfig:
    command: Optional[Tuple[str, ...]]
    output: str


@dataclass
class BenchmarkScenarioConfig:
    identifier: str
    name: str
    dataset: Optional[str]
    rendering_preset: Optional[str]
    runtime_profile: Optional[str]
    engine: BenchmarkCommandConfig
    reference: BenchmarkCommandConfig
    metrics: Tuple[BenchmarkMetricConfig, ...]


@dataclass
class BenchmarkConfig:
    schema_version: int
    scenarios: Tuple[BenchmarkScenarioConfig, ...]


@dataclass
class TelemetryOutputConfig:
    kind: str
    path: Optional[str]


@dataclass
class TelemetryMetricConfig:
    name: str
    statistic: str


@dataclass
class TelemetrySamplingConfig:
    frame_interval: int
    include_debug_overlays: bool


@dataclass
class TelemetryConfig:
    schema_version: int
    outputs: Tuple[TelemetryOutputConfig, ...]
    metrics: Tuple[TelemetryMetricConfig, ...]
    sampling: Optional[TelemetrySamplingConfig]


@dataclass
class Ai004Configuration:
    datasets: DatasetManifest
    rendering: Optional[RenderingConfig]
    runtime: Optional[RuntimeConfig]
    benchmarks: Optional[BenchmarkConfig]
    telemetry: Optional[TelemetryConfig]


def load_dataset_manifest(
    path: Union[str, os.PathLike[str]],
    *,
    require_schema: bool | None = ...,
) -> DatasetManifest: ...


def load_configuration(
    path: Union[str, os.PathLike[str]],
    *,
    require_schema: bool | None = ...,
) -> Ai004Configuration: ...
