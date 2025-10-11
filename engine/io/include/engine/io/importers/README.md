# Engine IO Importers

## Current State

- Hosts format-specific helpers consumed by `engine_io`. Geometry importers/exporters cover OBJ/OFF/STL/PLY meshes, PLY/XYZ/PCD point clouds, and edgelist/PLY graphs.
- Provides an animation clip importer/exporter that wraps `engine::animation` JSON serialization so tools can persist and reload author-authored clips.

## Usage

- Include the relevant header (for example `<engine/io/importers/animation.hpp>`) and call the provided load/save helpers instead of reimplementing format detection.
- When adding new importers/exporters, document their scope here and register them through `geometry_io_registry` or equivalent module registries.

## TODO / Next Steps

- Extend animation importers with binary/glTF ingest paths once deformation data is defined.
- Harden geometry importers around binary payloads (binary PLY/PCD/STL) and add streaming-aware hooks for large assets.
