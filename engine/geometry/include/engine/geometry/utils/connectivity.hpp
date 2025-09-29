#pragma once

#include "engine/geometry/properties/property_handle.hpp"

namespace engine::geometry {
    // Connectivity utilities can be added here in the future

    struct VertexConnectivity {
        HalfedgeHandle halfedge{};
    };

    struct HalfedgeConnectivity {
        FaceHandle face{};
        VertexHandle vertex{};
        HalfedgeHandle next{};
        HalfedgeHandle prev{};
    };

    struct FaceConnectivity {
        HalfedgeHandle halfedge{};
    };
}