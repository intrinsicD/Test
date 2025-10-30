"""Ingest AI-004 dataset manifests into a reproducible asset cache."""

from __future__ import annotations

import argparse
import hashlib
import json
import shutil
from dataclasses import dataclass
from pathlib import Path
from typing import Iterable, Mapping, MutableMapping, Optional, Sequence

from python.engine3g.config_schema import (
    ConfigurationSchemaError,
    DatasetEntry,
    DatasetStatistics,
    EdgeLengthMetrics,
    FeaturePreservation,
    MeshMetrics,
    ParameterizationChart,
    ParameterizationSummary,
    RemeshingTargets,
    TriangleQualityStatistics,
    load_dataset_manifest,
)

__all__ = [
    "DatasetIngestionError",
    "DatasetFileMetadata",
    "DatasetIngestionResult",
    "ingest_manifest",
    "main",
]


class DatasetIngestionError(RuntimeError):
    """Raised when dataset manifests cannot be ingested."""


@dataclass(frozen=True)
class DatasetFileMetadata:
    """Metadata describing a dataset file handled during ingestion."""

    label: str
    source_path: Path
    size_bytes: int
    sha256: str
    destination_path: Optional[Path]
    expected_size_bytes: Optional[int]
    expected_sha256: Optional[str]
    verified: bool


@dataclass(frozen=True)
class DatasetIngestionResult:
    """Summary of a dataset entry processed from a manifest."""

    entry: DatasetEntry
    files: tuple[DatasetFileMetadata, ...]
    summary_path: Optional[Path]


def _resolve_file_path(base: Path, value: str) -> Path:
    path = Path(value)
    if not path.is_absolute():
        path = base / path
    return path


def _hash_file(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def _serialize_edge_metrics(metrics: EdgeLengthMetrics) -> Mapping[str, float]:
    return {"min": metrics.minimum, "mean": metrics.mean, "max": metrics.maximum}


def _serialize_triangle_quality(metrics: TriangleQualityStatistics) -> Mapping[str, float]:
    return {"min": metrics.minimum, "mean": metrics.mean, "max": metrics.maximum}


def _serialize_mesh_metrics(metrics: MeshMetrics) -> Mapping[str, object]:
    return {
        "vertices": metrics.vertices,
        "faces": metrics.faces,
        "edge_length": _serialize_edge_metrics(metrics.edge_length),
    }


def _serialize_feature_preservation(preservation: FeaturePreservation) -> Mapping[str, object]:
    return {
        "lock_boundary_edges": preservation.lock_boundary_edges,
        "lock_feature_edges": preservation.lock_feature_edges,
        "minimum_feature_angle_degrees": preservation.minimum_feature_angle_degrees,
    }


def _serialize_statistics(statistics: DatasetStatistics) -> Mapping[str, object]:
    result: MutableMapping[str, object] = {
        "iterations": statistics.iteration_count,
        "max_error": statistics.max_error,
        "min_edge_length": statistics.min_edge_length,
        "max_edge_length": statistics.max_edge_length,
        "max_surface_deviation": statistics.max_surface_deviation,
        "mean_surface_deviation": statistics.mean_surface_deviation,
        "rms_surface_deviation": statistics.rms_surface_deviation,
    }

    if statistics.split_count is not None:
        result["splits"] = statistics.split_count
    if statistics.collapse_count is not None:
        result["collapses"] = statistics.collapse_count
    if statistics.duration_ms is not None:
        result["duration_ms"] = statistics.duration_ms
    if statistics.triangle_count is not None:
        result["triangles"] = statistics.triangle_count
    if statistics.triangle_quality is not None:
        result["triangle_quality"] = _serialize_triangle_quality(statistics.triangle_quality)

    return result


def _serialize_parameterization(parameterization: Optional[ParameterizationSummary]) -> Optional[Mapping[str, object]]:
    if parameterization is None:
        return None

    charts: list[Mapping[str, object]] = []
    for chart in parameterization.charts:
        charts.append(_serialize_parameterization_chart(chart))

    result: MutableMapping[str, object] = {
        "mode": parameterization.mode,
        "texel_density": parameterization.texel_density,
        "chart_count": parameterization.chart_count,
        "average_stretch": parameterization.average_stretch,
        "max_stretch": parameterization.max_stretch,
        "fill_ratio": parameterization.fill_ratio,
        "total_seam_length": parameterization.total_seam_length,
        "charts": charts,
    }
    if parameterization.target_texel_density is not None:
        result["target_texel_density"] = parameterization.target_texel_density
    if parameterization.atlas_area is not None:
        result["atlas_area"] = parameterization.atlas_area
    if parameterization.total_chart_area is not None:
        result["total_chart_area"] = parameterization.total_chart_area
    return result


def _serialize_parameterization_chart(chart: ParameterizationChart) -> Mapping[str, object]:
    return {
        "index": chart.index,
        "min_uv": list(chart.min_uv),
        "max_uv": list(chart.max_uv),
        "translation": list(chart.translation),
        "scale": chart.scale,
        "area": chart.area,
        "boundary_length": chart.boundary_length,
    }


def _serialize_remeshing_targets(targets: RemeshingTargets) -> Mapping[str, object]:
    result: MutableMapping[str, object] = {}
    if targets.target_edge_length is not None:
        result["target_edge_length"] = targets.target_edge_length
    if targets.relative_edge_scale is not None:
        result["relative_edge_scale"] = targets.relative_edge_scale
    if targets.max_normal_deviation_degrees is not None:
        result["max_normal_deviation_degrees"] = targets.max_normal_deviation_degrees
    if targets.max_surface_deviation is not None:
        result["max_surface_deviation"] = targets.max_surface_deviation
    return result


def _build_summary(entry: DatasetEntry, files: Iterable[DatasetFileMetadata]) -> Mapping[str, object]:
    file_block: MutableMapping[str, object] = {}
    for metadata in files:
        file_block[metadata.label] = {
            "source_path": str(metadata.source_path),
            "size_bytes": metadata.size_bytes,
            "sha256": metadata.sha256,
            "expected_size_bytes": metadata.expected_size_bytes,
            "expected_sha256": metadata.expected_sha256,
            "copied_to": str(metadata.destination_path) if metadata.destination_path else None,
            "verified": metadata.verified,
        }

    summary: MutableMapping[str, object] = {
        "id": entry.identifier,
        "schema": {"id": entry.schema_id, "version": entry.schema_version},
        "kind": entry.kind,
        "tags": list(entry.tags),
        "remeshing_mode": entry.remeshing_mode,
        "source_mesh": entry.source_mesh,
        "output_mesh": entry.output_mesh,
        "feature_preservation": _serialize_feature_preservation(entry.feature_preservation),
        "metrics": {
            "input": _serialize_mesh_metrics(entry.input_metrics),
            "output": _serialize_mesh_metrics(entry.output_metrics),
        },
        "statistics": _serialize_statistics(entry.statistics),
        "files": file_block,
    }

    if entry.remeshing_targets is not None:
        summary["remeshing_targets"] = _serialize_remeshing_targets(entry.remeshing_targets)

    parameterization = _serialize_parameterization(entry.parameterization)
    if parameterization is not None:
        summary["parameterization"] = parameterization

    if entry.job_label is not None:
        summary["job_label"] = entry.job_label

    return summary


def ingest_manifest(
    manifest_path: Path,
    destination_root: Path,
    *,
    copy_assets: bool = False,
    dry_run: bool = False,
    require_schema: bool | None = None,
) -> tuple[DatasetIngestionResult, ...]:
    """Ingest *manifest_path* into *destination_root* and return processed datasets."""

    manifest_path = manifest_path.resolve()
    if not manifest_path.is_file():
        raise DatasetIngestionError(f"manifest '{manifest_path}' does not exist")

    try:
        manifest = load_dataset_manifest(manifest_path, require_schema=require_schema)
    except ConfigurationSchemaError as error:
        raise DatasetIngestionError(str(error)) from error

    base_dir = manifest_path.parent
    results: list[DatasetIngestionResult] = []

    for entry in manifest.datasets:
        dataset_dir = destination_root / entry.identifier
        files: list[DatasetFileMetadata] = []
        for label, relative_path, expected_size, expected_sha in (
            (
                "source",
                entry.source_mesh,
                entry.source_mesh_size_bytes,
                entry.source_mesh_sha256,
            ),
            (
                "output",
                entry.output_mesh,
                entry.output_mesh_size_bytes,
                entry.output_mesh_sha256,
            ),
        ):
            file_path = _resolve_file_path(base_dir, relative_path)
            if not file_path.is_file():
                raise DatasetIngestionError(
                    f"dataset '{entry.identifier}' references missing file '{relative_path}' (resolved to '{file_path}')"
                )
            size_bytes = file_path.stat().st_size
            sha256 = _hash_file(file_path)
            if expected_sha is not None and sha256.lower() != expected_sha:
                raise DatasetIngestionError(
                    "dataset '{identifier}' {label} mesh hash mismatch: expected {expected} but computed {actual}".format(
                        identifier=entry.identifier,
                        label=label,
                        expected=expected_sha,
                        actual=sha256,
                    )
                )
            if expected_size is not None and size_bytes != expected_size:
                raise DatasetIngestionError(
                    "dataset '{identifier}' {label} mesh size mismatch: expected {expected} bytes but observed {actual}".format(
                        identifier=entry.identifier,
                        label=label,
                        expected=expected_size,
                        actual=size_bytes,
                    )
                )
            destination_path: Optional[Path] = None

            if copy_assets and not dry_run:
                dataset_dir.mkdir(parents=True, exist_ok=True)
                destination_path = dataset_dir / file_path.name
                shutil.copy2(file_path, destination_path)

            files.append(
                DatasetFileMetadata(
                    label=label,
                    source_path=file_path,
                    size_bytes=size_bytes,
                    sha256=sha256,
                    destination_path=destination_path,
                    expected_size_bytes=expected_size,
                    expected_sha256=expected_sha,
                    verified=(
                        (expected_sha is None or sha256.lower() == expected_sha)
                        and (expected_size is None or size_bytes == expected_size)
                    ),
                )
            )

        summary_path: Optional[Path] = None
        if not dry_run:
            dataset_dir.mkdir(parents=True, exist_ok=True)
            summary = _build_summary(entry, files)
            summary_path = dataset_dir / "ingestion.json"
            try:
                summary_path.write_text(json.dumps(summary, indent=2), encoding="utf-8")
            except OSError as error:
                raise DatasetIngestionError(f"failed to write ingestion summary '{summary_path}': {error}") from error

        results.append(DatasetIngestionResult(entry=entry, files=tuple(files), summary_path=summary_path))

    return tuple(results)


def _format_result(result: DatasetIngestionResult) -> str:
    copied = [f.label for f in result.files if f.destination_path]
    copied_summary = ", ".join(copied) if copied else "no files copied"
    verified = all(file.verified for file in result.files)
    return (
        f"dataset={result.entry.identifier} mode={result.entry.remeshing_mode} "
        f"files={len(result.files)} copied=[{copied_summary}] verified={'yes' if verified else 'no'}"
    )


def main(argv: Sequence[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("manifests", nargs="+", help="Dataset manifest paths to ingest")
    parser.add_argument(
        "--output",
        dest="output",
        default="artifacts/datasets",
        help="Destination root for ingested datasets (default: artifacts/datasets)",
    )
    parser.add_argument(
        "--copy-assets",
        dest="copy_assets",
        action="store_true",
        help="Copy referenced assets into the destination cache",
    )
    parser.add_argument(
        "--require-schema",
        dest="require_schema",
        action="store_true",
        help="Reject manifests that omit ai-004.schema headers",
    )
    parser.add_argument(
        "--dry-run",
        dest="dry_run",
        action="store_true",
        help="Validate manifests without copying files or writing summaries",
    )

    args = parser.parse_args(argv)
    destination = Path(args.output)

    try:
        for manifest_path in args.manifests:
            results = ingest_manifest(
                Path(manifest_path),
                destination,
                copy_assets=args.copy_assets,
                dry_run=args.dry_run,
                require_schema=args.require_schema,
            )
            for result in results:
                print(_format_result(result))
    except DatasetIngestionError as error:
        parser.error(str(error))
        return 2  # pragma: no cover - argparse already exits

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
