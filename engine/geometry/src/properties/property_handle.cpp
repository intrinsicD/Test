#include "engine/geometry/properties/property_handle.hpp"

#include <string>

namespace engine::geometry
{
    std::ostream& operator<<(std::ostream& os, VertexHandle v)
    {
        return os << 'v' << v.index();
    }

    std::ostream& operator<<(std::ostream& os, HalfedgeHandle h)
    {
        return os << 'h' << h.index();
    }

    std::ostream& operator<<(std::ostream& os, EdgeHandle e)
    {
        return os << 'e' << e.index();
    }

    std::ostream& operator<<(std::ostream& os, FaceHandle f)
    {
        return os << 'f' << f.index();
    }

    std::ostream& operator<<(std::ostream& os, NodeHandle n)
    {
        return os << 'n' << n.index();
    }
}
