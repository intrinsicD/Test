# Geometry Detection Fuzz Corpus

Store minimal, provenance-tracked seeds that exercise the geometry detection
harness (`engine_io_geometry_fuzz`). Keep entries small (<64 KiB) and document
format, intent, and source in this file. Update the table below whenever seeds
change.

| File                           | Format                | Intent                                                                          | Provenance                                                  |
|--------------------------------|-----------------------|---------------------------------------------------------------------------------|-------------------------------------------------------------|
| `mesh_triangle.obj`            | Wavefront OBJ mesh    | Baseline triangular mesh covering extension + signature heuristics.             | Synthetic sample authored for RT-006.2 harness integration. |
| `mesh_ascii.ply`               | ASCII PLY mesh        | Exercises PLY header parsing with vertex + face declarations.                   | Synthetic sample authored for RT-006.2 harness integration. |
| `mesh_ascii.stl`               | ASCII STL mesh        | Verifies ASCII STL signature detection.                                         | Synthetic sample authored for RT-006.2 harness integration. |
| `mesh_simple.off`              | OFF mesh              | Drives line-prefix detection for OFF headers.                                   | Synthetic sample authored for RT-006.2 harness integration. |
| `point_cloud_ascii.ply`        | ASCII PLY point cloud | Distinguishes vertex-only PLY data as point cloud.                              | Synthetic sample authored for RT-006.2 harness integration. |
| `point_cloud_ascii.pcd`        | ASCII PCD point cloud | Validates `.pcd` corpus path and header heuristics.                             | Synthetic sample authored for RT-006.2 harness integration. |
| `point_cloud_basic.xyz`        | XYZ point cloud       | Covers extension-driven point cloud classification.                             | Synthetic sample authored for RT-006.2 harness integration. |
| `graph_ascii.ply`              | ASCII PLY graph       | Confirms PLY edge declarations route to graph importers.                        | Synthetic sample authored for RT-006.2 harness integration. |
| `graph_edgelist.txt`           | Edge-list graph       | Exercises textual edge-list heuristic.                                          | Synthetic sample authored for RT-006.2 harness integration. |
| `invalid_truncated_header.ply` | Malformed PLY         | Confirms truncated headers classify as point clouds without crashing importers. | Synthetic sample authored for RT-006.2 harness integration. |
| `invalid_notply_header.ply`    | Malformed PLY         | Triggers `invalid_argument` error path when the magic header is missing.        | Synthetic sample authored for RT-006.2 harness integration. |

