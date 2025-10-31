"""Schema validation helpers for AI-004 configuration manifests."""

from __future__ import annotations

import json
import math
import os
import string
from copy import deepcopy
from dataclasses import dataclass
from pathlib import Path
from typing import Dict, Mapping, MutableMapping, MutableSequence, Optional, Sequence, Tuple, Union

try:  # pragma: no cover - optional dependency detection
    import yaml  # type: ignore
except ModuleNotFoundError:  # pragma: no cover - absence is handled at runtime
    yaml = None  # type: ignore

__all__ = [
    "ConfigurationSchemaError",
    "DatasetManifest",
    "DatasetEntry",
    "DatasetProvenance",
    "DatasetLicenseInfo",
    "DatasetProvenanceLink",
    "RemeshingTargets",
    "FeaturePreservation",
    "EdgeLengthMetrics",
    "MeshMetrics",
    "ParameterizationChart",
    "ParameterizationSummary",
    "TriangleQualityStatistics",
    "DatasetStatistics",
    "RenderingConfig",
    "RuntimeConfig",
    "RuntimeCameraConfig",
    "RuntimeSimulationConfig",
    "RuntimeHotReloadConfig",
    "BenchmarkConfig",
    "BenchmarkScenarioConfig",
    "BenchmarkCommandConfig",
    "BenchmarkMetricConfig",
    "BenchmarkThreshold",
    "TelemetryConfig",
    "TelemetryOutputConfig",
    "TelemetryMetricConfig",
    "TelemetrySamplingConfig",
    "Ai004Configuration",
    "load_dataset_manifest",
    "load_configuration",
]


class ConfigurationSchemaError(RuntimeError):
    """Raised when a configuration manifest fails schema validation."""


_SCHEMA_ENV_FLAG = "ENGINE_AI004_SCHEMA_V1"
_TRUTHY_VALUES = {"1", "true", "on", "yes", "enable", "enabled"}


def _is_schema_enforced(override: Optional[bool]) -> bool:
    """Return whether strict schema enforcement is enabled."""

    if override is not None:
        return override
    value = os.environ.get(_SCHEMA_ENV_FLAG)
    if value is None:
        return False
    return value.strip().lower() in _TRUTHY_VALUES


def _ensure_schema_header_defaults(target: MutableMapping[str, object], schema_id: str) -> None:
    """Populate default schema header entries when they are absent."""

    schema_value = target.get("schema")
    if not isinstance(schema_value, Mapping):
        target["schema"] = {"id": schema_id, "version": 1}
        return

    schema_dict: Dict[str, object] = dict(schema_value)
    schema_dict.setdefault("id", schema_id)
    schema_dict.setdefault("version", 1)
    target["schema"] = schema_dict


def _apply_dataset_schema_defaults(manifest: MutableMapping[str, object]) -> None:
    datasets_value = manifest.get("datasets")
    if not isinstance(datasets_value, Sequence) or isinstance(datasets_value, (str, bytes)):
        return
    for entry in datasets_value:
        if isinstance(entry, MutableMapping):
            _ensure_schema_header_defaults(entry, "ai-004.dataset")


def _apply_configuration_schema_defaults(configuration: MutableMapping[str, object]) -> None:
    datasets_value = configuration.get("datasets")
    if isinstance(datasets_value, MutableMapping):
        _apply_dataset_schema_defaults(datasets_value)
    elif isinstance(datasets_value, Sequence) and not isinstance(datasets_value, (str, bytes)):
        for entry in datasets_value:
            if isinstance(entry, MutableMapping):
                _ensure_schema_header_defaults(entry, "ai-004.dataset")

    for key, schema_id in (
        ("rendering", "ai-004.rendering"),
        ("runtime", "ai-004.runtime"),
        ("benchmarks", "ai-004.benchmarks"),
        ("telemetry", "ai-004.telemetry"),
    ):
        section = configuration.get(key)
        if isinstance(section, MutableMapping):
            _ensure_schema_header_defaults(section, schema_id)


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


def _require_positive_int(value: object, context: str) -> int:
    integer = _require_int(value, context)
    if integer <= 0:
        raise ConfigurationSchemaError(f"{context} must be greater than zero")
    return integer


def _require_non_negative_int(value: object, context: str) -> int:
    integer = _require_int(value, context)
    if integer < 0:
        raise ConfigurationSchemaError(f"{context} must be non-negative")
    return integer


def _require_float(value: object, context: str) -> float:
    if isinstance(value, (int, float)) and not isinstance(value, bool):
        result = float(value)
        if math.isfinite(result):
            return result
    raise ConfigurationSchemaError(f"{context} must be a finite number")


def _require_positive_float(value: object, context: str) -> float:
    number = _require_float(value, context)
    if number <= 0.0:
        raise ConfigurationSchemaError(f"{context} must be greater than zero")
    return number


def _require_non_negative_float(value: object, context: str) -> float:
    number = _require_float(value, context)
    if number < 0.0:
        raise ConfigurationSchemaError(f"{context} must be non-negative")
    return number


def _require_unit_interval_float(value: object, context: str) -> float:
    number = _require_float(value, context)
    if number < 0.0 or number > 1.0:
        raise ConfigurationSchemaError(f"{context} must be between 0 and 1 inclusive")
    return number


def _require_vec2(value: object, context: str) -> Tuple[float, float]:
    sequence = _require_sequence(value, context)
    if len(sequence) != 2:
        raise ConfigurationSchemaError(f"{context} must contain exactly two elements")
    return (
        _require_float(sequence[0], _child(context, 0)),
        _require_float(sequence[1], _child(context, 1)),
    )


def _require_vec3(value: object, context: str) -> Tuple[float, float, float]:
    sequence = _require_sequence(value, context)
    if len(sequence) != 3:
        raise ConfigurationSchemaError(f"{context} must contain exactly three elements")
    return (
        _require_float(sequence[0], _child(context, 0)),
        _require_float(sequence[1], _child(context, 1)),
        _require_float(sequence[2], _child(context, 2)),
    )


def _require_sha256(value: object, context: str) -> str:
    digest = _require_string(value, context).lower()
    if len(digest) != 64:
        raise ConfigurationSchemaError(f"{context} must contain a 64-character SHA-256 digest")
    if any(character not in string.hexdigits for character in digest):
        raise ConfigurationSchemaError(f"{context} must contain only hexadecimal characters")
    return digest


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
    surface_area: float
    edge_length: EdgeLengthMetrics

    @classmethod
    def from_mapping(cls, data: Mapping[str, object], context: str) -> "MeshMetrics":
        return cls(
            vertices=_require_int(data.get("vertices"), _child(context, "vertices")),
            faces=_require_int(data.get("faces"), _child(context, "faces")),
            surface_area=_require_non_negative_float(
                data.get("surface_area"), _child(context, "surface_area")
            ),
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
class TriangleQualityStatistics:
    minimum: float
    mean: float
    maximum: float

    @classmethod
    def from_mapping(cls, data: Mapping[str, object], context: str) -> "TriangleQualityStatistics":
        minimum = _require_unit_interval_float(data.get("min"), _child(context, "min"))
        mean = _require_unit_interval_float(data.get("mean"), _child(context, "mean"))
        maximum = _require_unit_interval_float(data.get("max"), _child(context, "max"))
        if minimum > maximum:
            raise ConfigurationSchemaError(
                f"{_child(context, 'min')} must be less than or equal to {_child(context, 'max')}"
            )
        if mean < minimum or mean > maximum:
            raise ConfigurationSchemaError(
                f"{_child(context, 'mean')} must lie between {_child(context, 'min')} and {_child(context, 'max')}"
            )
        return cls(minimum=minimum, mean=mean, maximum=maximum)


@dataclass(frozen=True)
class DatasetStatistics:
    iteration_count: int
    split_count: Optional[int]
    collapse_count: Optional[int]
    duration_ms: Optional[float]
    max_error: float
    min_edge_length: float
    max_edge_length: float
    max_surface_deviation: float
    mean_surface_deviation: float
    rms_surface_deviation: float
    triangle_count: Optional[int]
    triangle_quality: Optional[TriangleQualityStatistics]

    @classmethod
    def from_mapping(cls, data: Mapping[str, object], context: str) -> "DatasetStatistics":
        splits = None
        if "splits" in data:
            splits = _require_non_negative_int(data.get("splits"), _child(context, "splits"))
        collapses = None
        if "collapses" in data:
            collapses = _require_non_negative_int(data.get("collapses"), _child(context, "collapses"))
        duration_ms = None
        if "duration_ms" in data:
            duration_ms = _require_non_negative_float(data.get("duration_ms"), _child(context, "duration_ms"))
        triangle_count = None
        if "triangles" in data:
            triangle_count = _require_non_negative_int(data.get("triangles"), _child(context, "triangles"))
        triangle_quality = None
        if "triangle_quality" in data:
            triangle_quality = TriangleQualityStatistics.from_mapping(
                _require_mapping(data.get("triangle_quality"), _child(context, "triangle_quality")),
                _child(context, "triangle_quality"),
            )
        return cls(
            iteration_count=_require_int(data.get("iterations"), _child(context, "iterations")),
            split_count=splits,
            collapse_count=collapses,
            duration_ms=duration_ms,
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
            triangle_count=triangle_count,
            triangle_quality=triangle_quality,
        )


@dataclass(frozen=True)
class DatasetLicenseInfo:
    name: str
    url: Optional[str]
    notes: Optional[str]

    @classmethod
    def from_mapping(cls, data: Mapping[str, object], context: str) -> "DatasetLicenseInfo":
        name = _require_string(data.get("name"), _child(context, "name"))
        url = None
        if "url" in data:
            url = _require_string(data.get("url"), _child(context, "url"))
        notes = None
        if "notes" in data:
            notes = _require_string(data.get("notes"), _child(context, "notes"))
        return cls(name=name, url=url, notes=notes)

    def to_mapping(self) -> Dict[str, object]:
        payload: Dict[str, object] = {"name": self.name}
        if self.url is not None:
            payload["url"] = self.url
        if self.notes is not None:
            payload["notes"] = self.notes
        return payload


@dataclass(frozen=True)
class DatasetProvenanceLink:
    label: Optional[str]
    url: str

    @classmethod
    def from_mapping(cls, data: Mapping[str, object], context: str) -> "DatasetProvenanceLink":
        url = _require_string(data.get("url"), _child(context, "url"))
        label = None
        if "label" in data:
            label = _require_string(data.get("label"), _child(context, "label"))
        return cls(label=label, url=url)

    def to_mapping(self) -> Dict[str, object]:
        payload: Dict[str, object] = {"url": self.url}
        if self.label is not None:
            payload["label"] = self.label
        return payload


@dataclass(frozen=True)
class DatasetProvenance:
    summary: str
    license: DatasetLicenseInfo
    source: Optional[str]
    attribution: Optional[str]
    links: Tuple[DatasetProvenanceLink, ...]

    @classmethod
    def from_mapping(cls, data: Mapping[str, object], context: str) -> "DatasetProvenance":
        summary = _require_string(data.get("summary"), _child(context, "summary"))
        license_info = DatasetLicenseInfo.from_mapping(
            _require_mapping(data.get("license"), _child(context, "license")),
            _child(context, "license"),
        )

        source = None
        if "source" in data:
            source = _require_string(data.get("source"), _child(context, "source"))

        attribution = None
        if "attribution" in data:
            attribution = _require_string(data.get("attribution"), _child(context, "attribution"))

        links_value: Optional[Sequence[object]] = None
        if "links" in data:
            links_value = _require_sequence(data.get("links"), _child(context, "links"))

        links: MutableSequence[DatasetProvenanceLink] = []
        if links_value is not None:
            for index, link in enumerate(links_value):
                links.append(
                    DatasetProvenanceLink.from_mapping(
                        _require_mapping(link, _child(context, f"links[{index}]")),
                        _child(context, f"links[{index}]"),
                    )
                )

        return cls(
            summary=summary,
            license=license_info,
            source=source,
            attribution=attribution,
            links=tuple(links),
        )

    def to_mapping(self) -> Dict[str, object]:
        payload: Dict[str, object] = {
            "summary": self.summary,
            "license": self.license.to_mapping(),
        }
        if self.source is not None:
            payload["source"] = self.source
        if self.attribution is not None:
            payload["attribution"] = self.attribution
        if self.links:
            payload["links"] = [link.to_mapping() for link in self.links]
        return payload


@dataclass(frozen=True)
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
    provenance: Optional[DatasetProvenance]
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

        provenance: Optional[DatasetProvenance] = None
        if "provenance" in data:
            provenance = DatasetProvenance.from_mapping(
                _require_mapping(data.get("provenance"), _child(context, "provenance")),
                _child(context, "provenance"),
            )

        source_mesh_sha256 = None
        if "mesh_sha256" in source:
            source_mesh_sha256 = _require_sha256(source.get("mesh_sha256"), _child(context, "source.mesh_sha256"))
        source_mesh_size_bytes = None
        if "mesh_size_bytes" in source:
            source_mesh_size_bytes = _require_non_negative_int(
                source.get("mesh_size_bytes"), _child(context, "source.mesh_size_bytes")
            )

        output_mesh_sha256 = None
        if "mesh_sha256" in outputs:
            output_mesh_sha256 = _require_sha256(outputs.get("mesh_sha256"), _child(context, "outputs.mesh_sha256"))
        output_mesh_size_bytes = None
        if "mesh_size_bytes" in outputs:
            output_mesh_size_bytes = _require_non_negative_int(
                outputs.get("mesh_size_bytes"), _child(context, "outputs.mesh_size_bytes")
            )

        if schema_version >= 2:
            if source_mesh_sha256 is None or source_mesh_size_bytes is None:
                raise ConfigurationSchemaError(
                    f"{_child(context, 'source')} must include mesh_sha256 and mesh_size_bytes when schema.version >= 2"
                )
            if output_mesh_sha256 is None or output_mesh_size_bytes is None:
                raise ConfigurationSchemaError(
                    f"{_child(context, 'outputs')} must include mesh_sha256 and mesh_size_bytes when schema.version >= 2"
                )
            if provenance is None:
                raise ConfigurationSchemaError(
                    f"{_child(context, 'provenance')} must be present when schema.version >= 2"
                )

        return cls(
            identifier=identifier,
            schema_id=schema_id,
            schema_version=schema_version,
            kind=kind,
            tags=tuple(tags),
            source_generator=_require_string(source.get("generator"), _child(context, "source.generator")),
            source_mesh=_require_string(source.get("mesh"), _child(context, "source.mesh")),
            source_mesh_sha256=source_mesh_sha256,
            source_mesh_size_bytes=source_mesh_size_bytes,
            output_mesh=_require_string(outputs.get("mesh"), _child(context, "outputs.mesh")),
            output_mesh_sha256=output_mesh_sha256,
            output_mesh_size_bytes=output_mesh_size_bytes,
            remeshing_mode=remeshing_mode,
            remeshing_targets=targets,
            feature_preservation=feature_preservation,
            input_metrics=input_metrics,
            output_metrics=output_metrics,
            parameterization=parameterization,
            statistics=statistics,
            provenance=provenance,
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
        seen_identifiers: set[str] = set()
        for index, entry in enumerate(datasets_value):
            context = f"datasets[{index}]"
            dataset_entry = DatasetEntry.from_mapping(_require_mapping(entry, context), context)
            if dataset_entry.identifier in seen_identifiers:
                raise ConfigurationSchemaError(
                    f"{context}.id duplicates dataset identifier '{dataset_entry.identifier}'"
                )
            seen_identifiers.add(dataset_entry.identifier)
            datasets.append(dataset_entry)
        return cls(datasets=tuple(datasets))


def _parse_schema_header(data: Mapping[str, object], context: str, expected_id: str) -> int:
    schema = _require_mapping(data.get("schema"), _child(context, "schema"))
    schema_id = _require_string(schema.get("id"), _child(context, "schema.id"))
    if schema_id != expected_id:
        raise ConfigurationSchemaError(
            f"{_child(context, 'schema.id')} must be '{expected_id}'; received '{schema_id}'"
        )
    version = _require_int(schema.get("version"), _child(context, "schema.version"))
    if version < 1:
        raise ConfigurationSchemaError(f"{_child(context, 'schema.version')} must be >= 1")
    return version


@dataclass(frozen=True)
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

    @classmethod
    def from_mapping(cls, data: Mapping[str, object], context: str) -> "RenderingConfig":
        version = _parse_schema_header(data, context, "ai-004.rendering")
        preset = _require_string(data.get("preset"), _child(context, "preset"))
        options = data.get("options")
        shading_mode = "deferred"
        width = 1920
        height = 1080
        overlay_normals = False
        overlay_uv = False
        overlay_material = False
        overlay_light_volume = False

        if options is not None:
            options_map = _require_mapping(options, _child(context, "options"))
            if "shading_mode" in options_map:
                shading_value = _require_string(options_map.get("shading_mode"), _child(context, "options.shading_mode"))
                shading_lower = shading_value.lower()
                if shading_lower not in {"forward", "deferred"}:
                    raise ConfigurationSchemaError(
                        f"{_child(context, 'options.shading_mode')} must be 'forward' or 'deferred'; received '{shading_value}'"
                    )
                shading_mode = shading_lower
            if "resolution" in options_map:
                resolution = _require_mapping(options_map.get("resolution"), _child(context, "options.resolution"))
                width = _require_positive_int(resolution.get("width"), _child(context, "options.resolution.width"))
                height = _require_positive_int(resolution.get("height"), _child(context, "options.resolution.height"))
            if "overlays" in options_map:
                overlays = _require_mapping(options_map.get("overlays"), _child(context, "options.overlays"))
                if "normals" in overlays:
                    overlay_normals = _require_bool(overlays.get("normals"), _child(context, "options.overlays.normals"))
                if "uv" in overlays:
                    overlay_uv = _require_bool(overlays.get("uv"), _child(context, "options.overlays.uv"))
                if "material" in overlays:
                    overlay_material = _require_bool(overlays.get("material"), _child(context, "options.overlays.material"))
                if "light_volume" in overlays:
                    overlay_light_volume = _require_bool(
                        overlays.get("light_volume"), _child(context, "options.overlays.light_volume")
                    )

        return cls(
            schema_version=version,
            preset=preset,
            shading_mode=shading_mode,
            width=width,
            height=height,
            overlay_normals=overlay_normals,
            overlay_uv=overlay_uv,
            overlay_material=overlay_material,
            overlay_light_volume=overlay_light_volume,
        )


@dataclass(frozen=True)
class RuntimeCameraConfig:
    mode: str
    position: Optional[Tuple[float, float, float]]
    target: Optional[Tuple[float, float, float]]

    @classmethod
    def from_mapping(cls, data: Mapping[str, object], context: str) -> "RuntimeCameraConfig":
        mode = _require_string(data.get("mode"), _child(context, "mode"))
        mode_lower = mode.lower()
        if mode_lower not in {"orbit", "fly", "fixed"}:
            raise ConfigurationSchemaError(
                f"{_child(context, 'mode')} must be one of 'orbit', 'fly', or 'fixed'; received '{mode}'"
            )
        position = None
        if "position" in data:
            position = _require_vec3(data.get("position"), _child(context, "position"))
        target = None
        if "target" in data:
            target = _require_vec3(data.get("target"), _child(context, "target"))
        return cls(mode=mode_lower, position=position, target=target)


@dataclass(frozen=True)
class RuntimeSimulationConfig:
    timestep_seconds: float
    max_substeps: int

    @classmethod
    def from_mapping(cls, data: Mapping[str, object], context: str) -> "RuntimeSimulationConfig":
        return cls(
            timestep_seconds=_require_positive_float(
                data.get("timestep_seconds"), _child(context, "timestep_seconds")
            ),
            max_substeps=_require_positive_int(data.get("max_substeps"), _child(context, "max_substeps")),
        )


@dataclass(frozen=True)
class RuntimeHotReloadConfig:
    enabled: bool
    watch_interval_seconds: Optional[float]

    @classmethod
    def from_mapping(cls, data: Mapping[str, object], context: str) -> "RuntimeHotReloadConfig":
        enabled = _require_bool(data.get("enabled"), _child(context, "enabled"))
        interval = None
        if "watch_interval_seconds" in data:
            interval = _require_positive_float(data.get("watch_interval_seconds"), _child(context, "watch_interval_seconds"))
        return cls(enabled=enabled, watch_interval_seconds=interval)


@dataclass(frozen=True)
class RuntimeConfig:
    schema_version: int
    dataset: Optional[str]
    scene_manifest: Optional[str]
    scene_entry_point: Optional[str]
    camera: Optional[RuntimeCameraConfig]
    simulation: Optional[RuntimeSimulationConfig]
    hot_reload: RuntimeHotReloadConfig

    @classmethod
    def from_mapping(cls, data: Mapping[str, object], context: str) -> "RuntimeConfig":
        version = _parse_schema_header(data, context, "ai-004.runtime")
        dataset = None
        if "dataset" in data:
            dataset = _require_slug(data.get("dataset"), _child(context, "dataset"))

        scene_manifest = None
        scene_entry_point = None
        if "scene" in data:
            scene = _require_mapping(data.get("scene"), _child(context, "scene"))
            scene_manifest = _require_string(scene.get("manifest"), _child(context, "scene.manifest"))
            if "entry_point" in scene:
                scene_entry_point = _require_string(scene.get("entry_point"), _child(context, "scene.entry_point"))

        camera = None
        if "camera" in data:
            camera = RuntimeCameraConfig.from_mapping(
                _require_mapping(data.get("camera"), _child(context, "camera")), _child(context, "camera")
            )

        simulation = None
        if "simulation" in data:
            simulation = RuntimeSimulationConfig.from_mapping(
                _require_mapping(data.get("simulation"), _child(context, "simulation")),
                _child(context, "simulation"),
            )

        hot_reload_config = RuntimeHotReloadConfig(enabled=False, watch_interval_seconds=None)
        if "hot_reload" in data:
            hot_reload_config = RuntimeHotReloadConfig.from_mapping(
                _require_mapping(data.get("hot_reload"), _child(context, "hot_reload")),
                _child(context, "hot_reload"),
            )

        return cls(
            schema_version=version,
            dataset=dataset,
            scene_manifest=scene_manifest,
            scene_entry_point=scene_entry_point,
            camera=camera,
            simulation=simulation,
            hot_reload=hot_reload_config,
        )


@dataclass(frozen=True)
class BenchmarkThreshold:
    mode: str
    limit: float

    @classmethod
    def from_mapping(cls, data: Mapping[str, object], context: str) -> "BenchmarkThreshold":
        threshold_type = _require_string(data.get("type"), _child(context, "type"))
        threshold_lower = threshold_type.lower()
        if threshold_lower == "relative":
            limit = _require_non_negative_float(data.get("max_regression"), _child(context, "max_regression"))
        elif threshold_lower == "absolute":
            limit = _require_non_negative_float(data.get("max_delta"), _child(context, "max_delta"))
        else:
            raise ConfigurationSchemaError(
                f"{_child(context, 'type')} must be 'relative' or 'absolute'; received '{threshold_type}'"
            )
        return cls(mode=threshold_lower, limit=limit)


@dataclass(frozen=True)
class BenchmarkMetricConfig:
    name: str
    higher_is_better: bool
    threshold: BenchmarkThreshold

    @classmethod
    def from_mapping(cls, data: Mapping[str, object], context: str) -> "BenchmarkMetricConfig":
        name = _require_string(data.get("name"), _child(context, "name"))
        higher = _require_bool(data.get("higher_is_better"), _child(context, "higher_is_better"))
        threshold = BenchmarkThreshold.from_mapping(
            _require_mapping(data.get("threshold"), _child(context, "threshold")),
            _child(context, "threshold"),
        )
        return cls(name=name, higher_is_better=higher, threshold=threshold)


@dataclass(frozen=True)
class BenchmarkCommandConfig:
    command: Optional[Tuple[str, ...]]
    output: str

    @classmethod
    def from_mapping(cls, data: Mapping[str, object], context: str) -> "BenchmarkCommandConfig":
        output = _require_string(data.get("output"), _child(context, "output"))
        command_tokens: Optional[Tuple[str, ...]] = None
        if "command" in data:
            sequence = _require_sequence(data.get("command"), _child(context, "command"))
            if not sequence:
                raise ConfigurationSchemaError(f"{_child(context, 'command')} must not be empty")
            command_tokens = tuple(str(token) for token in sequence)
        return cls(command=command_tokens, output=output)


@dataclass(frozen=True)
class BenchmarkScenarioConfig:
    identifier: str
    name: str
    dataset: Optional[str]
    rendering_preset: Optional[str]
    runtime_profile: Optional[str]
    engine: BenchmarkCommandConfig
    reference: BenchmarkCommandConfig
    metrics: Tuple[BenchmarkMetricConfig, ...]

    @classmethod
    def from_mapping(cls, data: Mapping[str, object], context: str) -> "BenchmarkScenarioConfig":
        name = _require_string(data.get("name"), _child(context, "name"))
        identifier = _require_slug(data.get("id", name), _child(context, "id"))
        dataset = None
        if "dataset" in data:
            dataset = _require_slug(data.get("dataset"), _child(context, "dataset"))

        rendering_preset = None
        if "rendering_preset" in data:
            rendering_preset = _require_string(data.get("rendering_preset"), _child(context, "rendering_preset"))

        runtime_profile = None
        if "runtime_profile" in data:
            runtime_profile = _require_string(data.get("runtime_profile"), _child(context, "runtime_profile"))

        engine = BenchmarkCommandConfig.from_mapping(
            _require_mapping(data.get("engine"), _child(context, "engine")), _child(context, "engine")
        )
        reference = BenchmarkCommandConfig.from_mapping(
            _require_mapping(data.get("reference"), _child(context, "reference")),
            _child(context, "reference"),
        )

        metrics_value = _require_sequence(data.get("metrics"), _child(context, "metrics"))
        if not metrics_value:
            raise ConfigurationSchemaError(f"{_child(context, 'metrics')} must contain at least one metric")
        metrics: MutableSequence[BenchmarkMetricConfig] = []
        for index, entry in enumerate(metrics_value):
            metric_context = _child(context, f"metrics[{index}]")
            metrics.append(
                BenchmarkMetricConfig.from_mapping(_require_mapping(entry, metric_context), metric_context)
            )

        return cls(
            identifier=identifier,
            name=name,
            dataset=dataset,
            rendering_preset=rendering_preset,
            runtime_profile=runtime_profile,
            engine=engine,
            reference=reference,
            metrics=tuple(metrics),
        )


@dataclass(frozen=True)
class BenchmarkConfig:
    schema_version: int
    scenarios: Tuple[BenchmarkScenarioConfig, ...]

    @classmethod
    def from_mapping(cls, data: Mapping[str, object], context: str) -> "BenchmarkConfig":
        version = _parse_schema_header(data, context, "ai-004.benchmarks")
        scenarios_value = _require_sequence(data.get("scenarios"), _child(context, "scenarios"))
        if not scenarios_value:
            raise ConfigurationSchemaError(f"{_child(context, 'scenarios')} must contain at least one scenario")
        scenarios: MutableSequence[BenchmarkScenarioConfig] = []
        for index, entry in enumerate(scenarios_value):
            scenario_context = _child(context, f"scenarios[{index}]")
            scenarios.append(
                BenchmarkScenarioConfig.from_mapping(_require_mapping(entry, scenario_context), scenario_context)
            )
        return cls(schema_version=version, scenarios=tuple(scenarios))


@dataclass(frozen=True)
class TelemetryOutputConfig:
    kind: str
    path: Optional[str]

    @classmethod
    def from_mapping(cls, data: Mapping[str, object], context: str) -> "TelemetryOutputConfig":
        kind = _require_string(data.get("type"), _child(context, "type"))
        kind_lower = kind.lower()
        path = None
        if kind_lower == "file":
            path = _require_string(data.get("path"), _child(context, "path"))
        elif kind_lower == "stdout":
            if "path" in data:
                raise ConfigurationSchemaError(f"{_child(context, 'path')} is not valid for stdout outputs")
        else:
            raise ConfigurationSchemaError(
                f"{_child(context, 'type')} must be 'file' or 'stdout'; received '{kind}'"
            )
        return cls(kind=kind_lower, path=path)


@dataclass(frozen=True)
class TelemetryMetricConfig:
    name: str
    statistic: str

    @classmethod
    def from_mapping(cls, data: Mapping[str, object], context: str) -> "TelemetryMetricConfig":
        name = _require_string(data.get("name"), _child(context, "name"))
        statistic = _require_string(data.get("statistic"), _child(context, "statistic"))
        statistic_lower = statistic.lower()
        if statistic_lower not in {"mean", "median", "min", "max", "p95", "p99"}:
            raise ConfigurationSchemaError(
                f"{_child(context, 'statistic')} must be one of mean, median, min, max, p95, or p99; received '{statistic}'"
            )
        return cls(name=name, statistic=statistic_lower)


@dataclass(frozen=True)
class TelemetrySamplingConfig:
    frame_interval: int
    include_debug_overlays: bool

    @classmethod
    def from_mapping(cls, data: Mapping[str, object], context: str) -> "TelemetrySamplingConfig":
        frame_interval = _require_positive_int(data.get("frame_interval"), _child(context, "frame_interval"))
        include_debug = False
        if "include_debug_overlays" in data:
            include_debug = _require_bool(data.get("include_debug_overlays"), _child(context, "include_debug_overlays"))
        return cls(frame_interval=frame_interval, include_debug_overlays=include_debug)


@dataclass(frozen=True)
class TelemetryConfig:
    schema_version: int
    outputs: Tuple[TelemetryOutputConfig, ...]
    metrics: Tuple[TelemetryMetricConfig, ...]
    sampling: Optional[TelemetrySamplingConfig]

    @classmethod
    def from_mapping(cls, data: Mapping[str, object], context: str) -> "TelemetryConfig":
        version = _parse_schema_header(data, context, "ai-004.telemetry")

        outputs: Tuple[TelemetryOutputConfig, ...] = tuple()
        if "outputs" in data:
            outputs_value = _require_sequence(data.get("outputs"), _child(context, "outputs"))
            parsed_outputs: MutableSequence[TelemetryOutputConfig] = []
            for index, entry in enumerate(outputs_value):
                output_context = _child(context, f"outputs[{index}]")
                parsed_outputs.append(
                    TelemetryOutputConfig.from_mapping(_require_mapping(entry, output_context), output_context)
                )
            outputs = tuple(parsed_outputs)

        metrics: Tuple[TelemetryMetricConfig, ...] = tuple()
        if "metrics" in data:
            metrics_value = _require_sequence(data.get("metrics"), _child(context, "metrics"))
            parsed_metrics: MutableSequence[TelemetryMetricConfig] = []
            for index, entry in enumerate(metrics_value):
                metric_context = _child(context, f"metrics[{index}]")
                parsed_metrics.append(
                    TelemetryMetricConfig.from_mapping(_require_mapping(entry, metric_context), metric_context)
                )
            metrics = tuple(parsed_metrics)

        sampling = None
        if "sampling" in data:
            sampling = TelemetrySamplingConfig.from_mapping(
                _require_mapping(data.get("sampling"), _child(context, "sampling")),
                _child(context, "sampling"),
            )

        return cls(schema_version=version, outputs=outputs, metrics=metrics, sampling=sampling)


@dataclass(frozen=True)
class Ai004Configuration:
    datasets: DatasetManifest
    rendering: Optional[RenderingConfig]
    runtime: Optional[RuntimeConfig]
    benchmarks: Optional[BenchmarkConfig]
    telemetry: Optional[TelemetryConfig]

    @classmethod
    def from_mapping(cls, data: Mapping[str, object]) -> "Ai004Configuration":
        datasets = DatasetManifest(datasets=tuple())
        if "datasets" in data:
            datasets = DatasetManifest.from_mapping(data)

        dataset_slugs = {entry.identifier for entry in datasets.datasets}

        def _validate_dataset_reference(slug: str, context: str) -> None:
            if slug in dataset_slugs:
                return
            if dataset_slugs:
                available = ", ".join(sorted(dataset_slugs))
                raise ConfigurationSchemaError(
                    f"{context} references unknown dataset '{slug}'. Available datasets: {available}"
                )
            raise ConfigurationSchemaError(
                f"{context} references dataset '{slug}' but no datasets are declared"
            )

        rendering = None
        if "rendering" in data:
            rendering = RenderingConfig.from_mapping(
                _require_mapping(data.get("rendering"), "rendering"),
                "rendering",
            )

        runtime = None
        if "runtime" in data:
            runtime = RuntimeConfig.from_mapping(
                _require_mapping(data.get("runtime"), "runtime"),
                "runtime",
            )
            if runtime.dataset is not None:
                _validate_dataset_reference(runtime.dataset, "runtime.dataset")

        benchmarks = None
        if "benchmarks" in data:
            benchmarks = BenchmarkConfig.from_mapping(
                _require_mapping(data.get("benchmarks"), "benchmarks"),
                "benchmarks",
            )
            for index, scenario in enumerate(benchmarks.scenarios):
                if scenario.dataset is not None:
                    _validate_dataset_reference(
                        scenario.dataset,
                        f"benchmarks.scenarios[{index}].dataset",
                    )

        telemetry = None
        if "telemetry" in data:
            telemetry = TelemetryConfig.from_mapping(
                _require_mapping(data.get("telemetry"), "telemetry"),
                "telemetry",
            )

        return cls(
            datasets=datasets,
            rendering=rendering,
            runtime=runtime,
            benchmarks=benchmarks,
            telemetry=telemetry,
        )


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


def load_dataset_manifest(
    path: Union[str, os.PathLike[str]],
    *,
    require_schema: bool | None = None,
) -> DatasetManifest:
    """Load and validate a dataset manifest compatible with AI-004 schemas."""

    manifest_path = Path(path)
    raw = _load_raw_manifest(manifest_path)
    data = deepcopy(raw)
    mapping = _require_mapping(data, "manifest")

    if not _is_schema_enforced(require_schema) and isinstance(mapping, MutableMapping):
        _apply_dataset_schema_defaults(mapping)

    return DatasetManifest.from_mapping(mapping)


def load_configuration(
    path: Union[str, os.PathLike[str]],
    *,
    require_schema: bool | None = None,
) -> Ai004Configuration:
    """Load a complete AI-004 configuration manifest covering all schema sections."""

    manifest_path = Path(path)
    raw = _load_raw_manifest(manifest_path)
    data = deepcopy(raw)
    mapping = _require_mapping(data, "configuration")

    if not _is_schema_enforced(require_schema) and isinstance(mapping, MutableMapping):
        _apply_configuration_schema_defaults(mapping)

    return Ai004Configuration.from_mapping(mapping)
