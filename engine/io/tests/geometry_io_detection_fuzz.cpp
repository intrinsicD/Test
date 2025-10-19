#include "engine/io/geometry_io.hpp"

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <vector>

namespace
{
    [[nodiscard]] std::filesystem::path fuzz_input_path()
    {
        static const auto path = std::filesystem::temp_directory_path() / "engine_io_detection_fuzz.tmp";
        return path;
    }

    void write_fuzz_input(const std::uint8_t* data, std::size_t size)
    {
        std::ofstream stream{fuzz_input_path(), std::ios::binary | std::ios::trunc};
        if (!stream)
        {
            return;
        }

        stream.write(reinterpret_cast<const char*>(data), static_cast<std::streamsize>(size));
    }

    void exercise_geometry_detection(const std::filesystem::path& path)
    {
        auto detection = engine::io::detect_geometry_file(path);
        if (!detection)
        {
            return;
        }

        const auto& info = detection.value();
        try
        {
            switch (info.kind)
            {
            case engine::io::GeometryKind::mesh:
            {
                engine::geometry::Mesh mesh;
                (void)engine::io::read_mesh(path, mesh.interface, info.mesh_format);
                break;
            }
            case engine::io::GeometryKind::point_cloud:
            {
                engine::geometry::PointCloud point_cloud;
                (void)engine::io::read_point_cloud(path, point_cloud.interface, info.point_cloud_format);
                break;
            }
            case engine::io::GeometryKind::graph:
            {
                engine::geometry::Graph graph;
                (void)engine::io::read_graph(path, graph.interface, info.graph_format);
                break;
            }
            case engine::io::GeometryKind::unknown:
                break;
            }
        }
        catch (...) // NOLINT(bugprone-empty-catch)
        {
            // The IO pipeline communicates structured errors through exceptions when the
            // parser encounters invalid data. Swallowing them keeps the fuzzing harness
            // focused on detecting crashes rather than expected validation failures.
        }
    }
} // namespace

extern "C" int LLVMFuzzerTestOneInput(const std::uint8_t* data, std::size_t size)
{
    if (data == nullptr)
    {
        return 0;
    }

    write_fuzz_input(data, size);
    exercise_geometry_detection(fuzz_input_path());
    return 0;
}

#ifndef ENGINE_IO_FUZZ_WITH_LIBFUZZER
int main(int argc, char** argv)
{
    if (argc < 2)
    {
        return 0;
    }

    const std::filesystem::path input_path{argv[1]};
    std::ifstream file{input_path, std::ios::binary};
    if (!file)
    {
        return 0;
    }

    std::vector<std::uint8_t> buffer(static_cast<std::size_t>(std::filesystem::file_size(input_path)));
    file.read(reinterpret_cast<char*>(buffer.data()), static_cast<std::streamsize>(buffer.size()));
    LLVMFuzzerTestOneInput(buffer.data(), buffer.size());
    return 0;
}
#endif
