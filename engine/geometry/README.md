# engine/geometry

This directory collects the C++ engine source tree, the geometry subsystem for meshes, primitives, and spatial algorithms.
In addition to the spatial primitives, it now exposes the property registry that tracks per-element attributes without relying on exceptions, making it suitable for downstream mesh processing tools.
Future additions should place additional contributions to the geometry subsystem for meshes, primitives, and spatial algorithms here to keep related work easy to discover.
