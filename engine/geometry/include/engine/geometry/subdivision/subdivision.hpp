#pragma once
namespace engine::geometry
{
    struct HalfedgeMeshInterface;
}

namespace engine::geometry::subdivision::approximation
{
    void CatmullClark(const HalfedgeMeshInterface& input_mesh, HalfedgeMeshInterface& output_mesh, int iterations = 1);
    void DooSabin(const HalfedgeMeshInterface& input_mesh, HalfedgeMeshInterface& output_mesh, int iterations = 1);
    void Loop(const HalfedgeMeshInterface& input_mesh, HalfedgeMeshInterface& output_mesh, int iterations = 1);
    void sqrt3(const HalfedgeMeshInterface& input_mesh, HalfedgeMeshInterface& output_mesh, int iterations = 1);
    void velho_zorin(const HalfedgeMeshInterface& input_mesh, HalfedgeMeshInterface& output_mesh, int iterations = 1);
    void midedge(const HalfedgeMeshInterface& input_mesh, HalfedgeMeshInterface& output_mesh, int iterations = 1);
} // namespace engine::geometry

namespace engine::geometry::subdivision::interpolating
{
    void butterfly(const HalfedgeMeshInterface& input_mesh, HalfedgeMeshInterface& output_mesh, int iterations = 1);
    void modified_butterfly(const HalfedgeMeshInterface& input_mesh, HalfedgeMeshInterface& output_mesh, int iterations = 1);
    void quads(const HalfedgeMeshInterface& input_mesh, HalfedgeMeshInterface& output_mesh, int iterations = 1);
    void sqrt3_interpolating(const HalfedgeMeshInterface& input_mesh, HalfedgeMeshInterface& output_mesh, int iterations = 1);
} // namespace engine::geometry
