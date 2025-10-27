"""Prototype harness implementation for AI-004 runtime workflows."""

from __future__ import annotations

from dataclasses import dataclass
from typing import Callable, Optional

from .config_schema import (
    Ai004Configuration,
    DatasetEntry,
    DatasetManifest,
    RenderingConfig,
    RuntimeConfig,
)
from .loader import EngineRuntimeHandle, EngineLibraryNotFound, load_runtime

__all__ = [
    "HarnessExecutionOptions",
    "HarnessRunSummary",
    "PrototypeHarness",
    "PrototypeHarnessError",
    "load_harness",
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


RuntimeFactory = Callable[[], EngineRuntimeHandle]


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

    def run_headless(self, options: HarnessExecutionOptions | None = None) -> HarnessRunSummary:
        """Execute a fixed-timestep runtime loop and return a summary."""

        execution = options or HarnessExecutionOptions()
        execution.validate()

        rendering = self._rendering_config()

        if execution.dry_run:
            return HarnessRunSummary(
                dataset_id=self._selected_dataset.identifier if self._selected_dataset else None,
                rendering_preset=rendering.preset if rendering else None,
                shading_mode=rendering.shading_mode if rendering else None,
                frames_executed=0,
                timestep_seconds=execution.dt,
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

        return HarnessRunSummary(
            dataset_id=self._selected_dataset.identifier if self._selected_dataset else None,
            rendering_preset=rendering.preset if rendering else None,
            shading_mode=rendering.shading_mode if rendering else None,
            frames_executed=frames_executed,
            timestep_seconds=execution.dt,
        )


def load_harness(
    path: str,
    *,
    runtime_factory: RuntimeFactory | None = None,
) -> PrototypeHarness:
    """Load an AI-004 configuration from *path* and construct a harness."""

    from .config_schema import load_configuration  # Local import to avoid cycle during module init

    configuration = load_configuration(path)
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
    return (
        f"dataset={dataset} preset={preset} shading={shading} "
        f"frames={summary.frames_executed} dt={summary.timestep_seconds:.6f}"
    )

