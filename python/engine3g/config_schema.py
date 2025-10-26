"""Schema validation helpers for AI-004 configuration manifests."""

from __future__ import annotations

from dataclasses import dataclass
from pathlib import Path
from typing import Mapping, MutableSequence, Optional, Sequence, Tuple, Union

import json
import math
import os
from copy import deepcopy

try:  # pragma: no cover - optional dependency detection
    import yaml  # type: ignore
except ModuleNotFoundError:  # pragma: no cover - absence is handled at runtime
    yaml = None  # type: ignore

__all__ = [
    "ConfigurationSchemaError",
    "DatasetManifest",
    "DatasetEntry",
    "RemeshingTargets",
    "FeaturePreservation",
    "EdgeLengthMetrics",
    "MeshMetrics",
    "ParameterizationChart",
    "ParameterizationSummary",
    "DatasetStatistics",
    "load_dataset_manifest",
]


class ConfigurationSchemaError(RuntimeError):
    """Raised when a configuration manifest fails schema validation."""


def _child(context: str, key: Union[str, int]) -> str:
    return f"{context}[{key}]" if isinstance(key, int) else (f"{context}.{key}" if context else str(key))


def _require_mapping(value: object, context: str) -> Mapping[str, object]:
    if not isinstance(value, Mapping):
        raise ConfigurationSchemaError(f"{context} must be a mapping; received {type(value).__name__}")
    return value  # type: ignore[return-value]


def _require_sequence(value: object, context: str) -> Sequence[object]:
    if not isinstance(value, Sequence) or isinstance(value, (str, bytes)):
        raise ConfigurationSchemaError(f"{context} must be a sequence; received {type(value).__name__}")
    return value  # type: ignore[return-value]


def _require_string(value: object, context: str) -> str:
    if not isinstance(value, str) or not value:
        raise ConfigurationSchemaError(f"{context} must be a non-empty string")
    return value


def _require_slug(value: object, context: str) -> str:
    text = _require_string(value, context)
    has_alphanumeric = False
    for ch in text:
        if ch.isalnum() and ch.lower() == ch:
            has_alphanumeric = True
            continue
        if ch == "-":
            continue
        raise ConfigurationSchemaError(
            f"{context} must contain lowercase alphanumeric characters separated by hyphens; received '{text}'"
        )
    if not has_alphanumeric:
        raise ConfigurationSchemaError(f"{context} must include at least one alphanumeric character; received '{text}'")
    if text[0] == "-" or text[-1] == "-":
        raise ConfigurationSchemaError(f"{context} must not start or end with a hyphen; received '{text}'")
    return text


def _require_bool(value: object, context: str) -> bool:
    if isinstance(value, bool):
        return value
    raise ConfigurationSchemaError(f"{context} must be a boolean")


def _require_int(value: object, context: str) -> int:
    if isinstance(value, bool) or not isinstance(value, int):
        raise ConfigurationSchemaError(f"{context} must be an integer")
    return value


def _require_float(value: object, context: str) -> float:
    if isinstance(value, (int, float)) and not isinstance(value, bool):
        result = float(value)
        if math.isfinite(result):
            return result
    raise ConfigurationSchemaError(f"{context} must be a finite number")


def _require_vec2(value: object, context: str) -> Tuple[float, float]:
    sequence = _require_sequence(value, context)
    if len(sequence) != 2:
        raise ConfigurationSchemaError(f"{context} must contain exactly two elements")
    return (_require_float(sequence[0], _child(context, 0)), _require_float(sequence[1], _child(context, 1)))


@dataclass(frozen=True)
class RemeshingTargets:
    target_edge_length: Optional[float] = None
    relative_edge_scale: Optional[float] = None
    max_normal_deviation_degrees: Optional[float] = None
    max_surface_deviation: Optional[float] = None

    @classmethod
    def from_mapping(cls, data: Mapping[str, object], context: str) -> "RemeshingTargets":
        target_edge_length = None
        relative_edge_scale = None
        max_normal_deviation_degrees = None
        max_surface_deviation = None

        if "target_edge_length" in data:
            target_edge_length = _require_float(data["target_edge_length"], _child(context, "target_edge_length"))
        if "relative_edge_scale" in data:
            relative_edge_scale = _require_float(data["relative_edge_scale"], _child(context, "relative_edge_scale"))
        if "max_normal_deviation_degrees" in data:
            max_normal_deviation_degrees = _require_float(
                data["max_normal_deviation_degrees"], _child(context, "max_normal_deviation_degrees")
            )
        if "max_surface_deviation" in data:
            max_surface_deviation = _require_float(data["max_surface_deviation"], _child(context, "max_surface_deviation"))
        return cls(
            target_edge_length=target_edge_length,
            relative_edge_scale=relative_edge_scale,
            max_normal_deviation_degrees=max_normal_deviation_degrees,
            max_surface_deviation=max_surface_deviation,
        )


@dataclass(frozen=True)
class FeaturePreservation:
    lock_boundary_edges: bool
    lock_feature_edges: bool
    minimum_feature_angle_degrees: float

    @classmethod
    def from_mapping(cls, data: Mapping[str, object], context: str) -> "FeaturePreservation":
        return cls(
            lock_boundary_edges=_require_bool(data.get("lock_boundary_edges"), _child(context, "lock_boundary_edges")),
            lock_feature_edges=_require_bool(data.get("lock_feature_edges"), _child(context, "lock_feature_edges")),
            minimum_feature_angle_degrees=_require_float(
                data.get("minimum_feature_angle_degrees"), _child(context, "minimum_feature_angle_degrees")
            ),
        )


@dataclass(frozen=True)
class EdgeLengthMetrics:
    minimum: float
    maximum: float
    mean: float

    @classmethod
    def from_mapping(cls, data: Mapping[str, object], context: str) -> "EdgeLengthMetrics":
        return cls(
            minimum=_require_float(data.get("min"), _child(context, "min")),
            maximum=_require_float(data.get("max"), _child(context, "max")),
            mean=_require_float(data.get("mean"), _child(context, "mean")),
        )


@dataclass(frozen=True)
class MeshMetrics:
    vertices: int
    faces: int
    edge_length: EdgeLengthMetrics

    @classmethod
    def from_mapping(cls, data: Mapping[str, object], context: str) -> "MeshMetrics":
        return cls(
            vertices=_require_int(data.get("vertices"), _child(context, "vertices")),
            faces=_require_int(data.get("faces"), _child(context, "faces")),
            edge_length=EdgeLengthMetrics.from_mapping(
                _require_mapping(data.get("edge_length"), _child(context, "edge_length")), _child(context, "edge_length")
            ),
        )


@dataclass(frozen=True)
class ParameterizationChart:
    index: int
    min_uv: Tuple[float, float]
    max_uv: Tuple[float, float]
    translation: Tuple[float, float]
    scale: float
    area: float
    boundary_length: float

    @classmethod
    def from_mapping(cls, data: Mapping[str, object], context: str) -> "ParameterizationChart":
        return cls(
            index=_require_int(data.get("index"), _child(context, "index")),
            min_uv=_require_vec2(data.get("min_uv"), _child(context, "min_uv")),
            max_uv=_require_vec2(data.get("max_uv"), _child(context, "max_uv")),
            translation=_require_vec2(data.get("translation"), _child(context, "translation")),
            scale=_require_float(data.get("scale"), _child(context, "scale")),
            area=_require_float(data.get("area"), _child(context, "area")),
            boundary_length=_require_float(data.get("boundary_length"), _child(context, "boundary_length")),
        )


@dataclass(frozen=True)
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

    @classmethod
    def from_mapping(cls, data: Mapping[str, object], context: str) -> "ParameterizationSummary":
        mode = _require_string(data.get("mode"), _child(context, "mode"))
        if mode not in {
            "none",
            "reuse_existing",
            "generate_lscm",
            "generate_abfpp",
        }:
            raise ConfigurationSchemaError(f"{_child(context, 'mode')} contains unsupported value '{mode}'")

        target_texel_density = None
        if "target_texel_density" in data:
            target_texel_density = _require_float(data.get("target_texel_density"), _child(context, "target_texel_density"))

        charts: MutableSequence[ParameterizationChart] = []
        if "charts" in data:
            sequence_value = data.get("charts")
            sequence = _require_sequence(sequence_value, _child(context, "charts"))
            for index, entry in enumerate(sequence):
                chart_context = _child(context, f"charts[{index}]")
                chart_mapping = _require_mapping(entry, chart_context)
                charts.append(ParameterizationChart.from_mapping(chart_mapping, chart_context))

        return cls(
            mode=mode,
            target_texel_density=target_texel_density,
            texel_density=_require_float(data.get("texel_density"), _child(context, "texel_density")),
            chart_count=_require_int(data.get("chart_count"), _child(context, "chart_count")),
            average_stretch=_require_float(data.get("average_stretch"), _child(context, "average_stretch")),
            max_stretch=_require_float(data.get("max_stretch"), _child(context, "max_stretch")),
            fill_ratio=_require_float(data.get("fill_ratio"), _child(context, "fill_ratio")),
            total_seam_length=_require_float(data.get("total_seam_length"), _child(context, "total_seam_length")),
            atlas_area=_require_float(data.get("atlas_area"), _child(context, "atlas_area"))
            if "atlas_area" in data
            else None,
            total_chart_area=_require_float(data.get("total_chart_area"), _child(context, "total_chart_area"))
            if "total_chart_area" in data
            else None,
            charts=tuple(charts),
        )


@dataclass(frozen=True)
class DatasetStatistics:
    iteration_count: int
    max_error: float
    min_edge_length: float
    max_edge_length: float
    max_surface_deviation: float
    mean_surface_deviation: float
    rms_surface_deviation: float

    @classmethod
    def from_mapping(cls, data: Mapping[str, object], context: str) -> "DatasetStatistics":
        return cls(
            iteration_count=_require_int(data.get("iterations"), _child(context, "iterations")),
            max_error=_require_float(data.get("max_error"), _child(context, "max_error")),
            min_edge_length=_require_float(data.get("min_edge_length"), _child(context, "min_edge_length")),
            max_edge_length=_require_float(data.get("max_edge_length"), _child(context, "max_edge_length")),
            max_surface_deviation=_require_float(
                data.get("max_surface_deviation"), _child(context, "max_surface_deviation")
            ),
            mean_surface_deviation=_require_float(
                data.get("mean_surface_deviation"), _child(context, "mean_surface_deviation")
            ),
            rms_surface_deviation=_require_float(
                data.get("rms_surface_deviation"), _child(context, "rms_surface_deviation")
            ),
        )


@dataclass(frozen=True)
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
    job_label: Optional[str] = None

    @classmethod
    def from_mapping(cls, data: Mapping[str, object], context: str) -> "DatasetEntry":
        schema = _require_mapping(data.get("schema"), _child(context, "schema"))
        schema_id = _require_string(schema.get("id"), _child(context, "schema.id"))
        if schema_id != "ai-004.dataset":
            raise ConfigurationSchemaError(
                f"{_child(context, 'schema.id')} must be 'ai-004.dataset'; received '{schema_id}'"
            )
        schema_version = _require_int(schema.get("version"), _child(context, "schema.version"))
        if schema_version < 1:
            raise ConfigurationSchemaError(f"{_child(context, 'schema.version')} must be >= 1")

        identifier = _require_slug(data.get("id"), _child(context, "id"))
        kind = _require_string(data.get("kind"), _child(context, "kind"))
        tags_value = _require_sequence(data.get("tags"), _child(context, "tags"))
        tags: MutableSequence[str] = []
        for index, tag in enumerate(tags_value):
            tags.append(_require_string(tag, _child(context, f"tags[{index}]")))

        remeshing = _require_mapping(data.get("remeshing"), _child(context, "remeshing"))
        remeshing_mode = _require_string(remeshing.get("mode"), _child(context, "remeshing.mode"))
        if remeshing_mode not in {"uniform", "feature_preserving", "adaptive"}:
            raise ConfigurationSchemaError(
                f"{_child(context, 'remeshing.mode')} contains unsupported value '{remeshing_mode}'"
            )

        targets = None
        if "targets" in remeshing:
            targets = RemeshingTargets.from_mapping(
                _require_mapping(remeshing.get("targets"), _child(context, "remeshing.targets")),
                _child(context, "remeshing.targets"),
            )

        feature_preservation = FeaturePreservation.from_mapping(
            _require_mapping(data.get("feature_preservation"), _child(context, "feature_preservation")),
            _child(context, "feature_preservation"),
        )

        metrics = _require_mapping(data.get("metrics"), _child(context, "metrics"))
        input_metrics = MeshMetrics.from_mapping(
            _require_mapping(metrics.get("input"), _child(context, "metrics.input")),
            _child(context, "metrics.input"),
        )
        output_metrics = MeshMetrics.from_mapping(
            _require_mapping(metrics.get("output"), _child(context, "metrics.output")),
            _child(context, "metrics.output"),
        )

        parameterization = None
        if "parameterization" in data:
            parameterization = ParameterizationSummary.from_mapping(
                _require_mapping(data.get("parameterization"), _child(context, "parameterization")),
                _child(context, "parameterization"),
            )

        statistics = DatasetStatistics.from_mapping(
            _require_mapping(data.get("statistics"), _child(context, "statistics")),
            _child(context, "statistics"),
        )

        source = _require_mapping(data.get("source"), _child(context, "source"))
        outputs = _require_mapping(data.get("outputs"), _child(context, "outputs"))

        return cls(
            identifier=identifier,
            schema_id=schema_id,
            schema_version=schema_version,
            kind=kind,
            tags=tuple(tags),
            source_generator=_require_string(source.get("generator"), _child(context, "source.generator")),
            source_mesh=_require_string(source.get("mesh"), _child(context, "source.mesh")),
            output_mesh=_require_string(outputs.get("mesh"), _child(context, "outputs.mesh")),
            remeshing_mode=remeshing_mode,
            remeshing_targets=targets,
            feature_preservation=feature_preservation,
            input_metrics=input_metrics,
            output_metrics=output_metrics,
            parameterization=parameterization,
            statistics=statistics,
            job_label=_require_string(data.get("job_label"), _child(context, "job_label"))
            if "job_label" in data
            else None,
        )


@dataclass(frozen=True)
class DatasetManifest:
    datasets: Tuple[DatasetEntry, ...]

    @classmethod
    def from_mapping(cls, data: Mapping[str, object]) -> "DatasetManifest":
        datasets_value = _require_sequence(data.get("datasets"), "datasets")
        datasets: MutableSequence[DatasetEntry] = []
        for index, entry in enumerate(datasets_value):
            context = f"datasets[{index}]"
            datasets.append(DatasetEntry.from_mapping(_require_mapping(entry, context), context))
        return cls(datasets=tuple(datasets))


def _load_raw_manifest(path: Path) -> Mapping[str, object]:
    text = path.read_text(encoding="utf-8")
    suffix = path.suffix.lower()
    if suffix in {".json", ""}:
        data = json.loads(text)
        if not isinstance(data, Mapping):
            raise ConfigurationSchemaError("JSON manifests must evaluate to an object")
        return data  # type: ignore[return-value]
    if suffix in {".yml", ".yaml"}:
        if yaml is None:
            raise ConfigurationSchemaError("PyYAML is required to load YAML manifests; install PyYAML or use JSON")
        data = yaml.safe_load(text)
        if not isinstance(data, Mapping):
            raise ConfigurationSchemaError("YAML manifests must evaluate to a mapping")
        return data  # type: ignore[return-value]
    raise ConfigurationSchemaError(f"Unsupported manifest format '{suffix}'")


def load_dataset_manifest(path: Union[str, os.PathLike[str]]) -> DatasetManifest:
    """Load and validate a dataset manifest compatible with AI-004 schemas."""

    manifest_path = Path(path)
    raw = _load_raw_manifest(manifest_path)
    data = deepcopy(raw)
    mapping = _require_mapping(data, "manifest")
    return DatasetManifest.from_mapping(mapping)
