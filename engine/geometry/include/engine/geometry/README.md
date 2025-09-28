# engine/geometry/include/engine/geometry

This directory collects the C++ engine source tree, the geometry subsystem for meshes, primitives, and spatial algorithms, public headers defining the subsystem API.
The type-erased attribute system lives in `property_registry.hpp` and `property_set.hpp`, enabling structured property access shared by mesh, graph, and point-set data structures.
General-purpose combinatorial graphs are available through `graph/graph.hpp`, and unstructured point sets are modelled by `point_cloud.hpp`, both exposing property accessors consistent with `mesh/halfedge_mesh.hpp`.
Future additions should place additional contributions to public headers defining the subsystem API here to keep related work easy to discover.
