from dataclasses import dataclass
from typing import Optional, Tuple, Union
import os


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


@dataclass
class DatasetEntry:
    identifier: str
    schema_id: str
    schema_version: int
    kind: str
    tags: Tuple[str, ...]
    source_generator: str
    source_mesh: str
    output_mesh: str
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


def load_dataset_manifest(path: Union[str, os.PathLike[str]]) -> DatasetManifest: ...
