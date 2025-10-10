#pragma once

#include <iosfwd>

namespace engine::scene
{
    class Scene;
}

namespace engine::scene::serialization
{
    void save(const Scene& scene, std::ostream& output);
    void load(Scene& scene, std::istream& input);
}
