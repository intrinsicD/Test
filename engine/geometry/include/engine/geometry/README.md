# engine/geometry/include/engine/geometry

This directory collects the C++ engine source tree, the geometry subsystem for meshes, primitives, and spatial algorithms, public headers defining the subsystem API.
The newly added `property_registry.hpp` header contains the type-erased registry that manages mesh and point-set attributes with optional-based discovery instead of throwing exceptions.
Future additions should place additional contributions to public headers defining the subsystem API here to keep related work easy to discover.
