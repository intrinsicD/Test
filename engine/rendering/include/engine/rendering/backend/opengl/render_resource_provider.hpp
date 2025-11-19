#pragma once

#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "engine/assets/handles.hpp"
#include "engine/geometry/api.hpp"
#include "engine/geometry/graph/graph.hpp"
#include "engine/geometry/point_cloud/point_cloud.hpp"
#include "engine/math/vector.hpp"
#include "engine/rendering/render_pass.hpp"

namespace engine::rendering::backend::opengl
{
    /**
     * \brief Materialises geometry resources for the OpenGL backend.
     *
     * The provider bridges render graph recording with runtime asset handles by
     * resolving meshes into CPU-resident buffers and, when available, uploading
     * them to GPU vertex/index buffers.  It implements the generic
     * RenderResourceProvider interface so render passes can demand residency of
     * assets before recording draw calls.
     */
    class OpenGLRenderResourceProvider final : public RenderResourceProvider
    {
    public:
        using MeshResolver = std::function<std::optional<geometry::SurfaceMesh>(
            const assets::MeshHandle& handle)>;
        using PointCloudResolver = std::function<std::optional<geometry::PointCloud>(
            const assets::PointCloudHandle& handle)>;
        using GraphResolver = std::function<std::optional<geometry::Graph>(
            const assets::GraphHandle& handle)>;

        struct MeshRecord
        {
            assets::MeshHandle handle{};
            std::vector<math::vec3> positions;
            std::vector<math::vec3> normals;
            std::vector<math::vec2> texture_coordinates;
            std::vector<std::uint32_t> indices;
            geometry::Aabb bounds{};
#if ENGINE_RENDERING_HAS_GLAD
            unsigned int vertex_array{0};
            unsigned int position_buffer{0};
            unsigned int normal_buffer{0};
            unsigned int texcoord_buffer{0};
            unsigned int index_buffer{0};
            bool gpu_uploaded{false};
#else
            bool gpu_uploaded{false};
#endif
        };

        explicit OpenGLRenderResourceProvider(MeshResolver mesh_resolver,
                                              PointCloudResolver point_cloud_resolver = {},
                                              GraphResolver graph_resolver = {});
        ~OpenGLRenderResourceProvider() override;

        void require_mesh(const assets::MeshHandle& handle) override;
        void require_mesh_async(const assets::MeshHandle& handle);
        void require_graph(const assets::GraphHandle& handle) override;
        void require_point_cloud(const assets::PointCloudHandle& handle) override;
        void require_material(const assets::MaterialHandle& handle) override;
        void require_shader(const assets::ShaderHandle& handle) override;

        void flush_pending_uploads();
        [[nodiscard]] std::size_t pending_async_uploads() const noexcept;

        [[nodiscard]] const MeshRecord* mesh(const assets::MeshHandle& handle) const noexcept;
        [[nodiscard]] std::size_t loaded_mesh_count() const noexcept;
        [[nodiscard]] std::size_t mesh_gpu_upload_count() const noexcept;

        struct PointCloudRecord
        {
            assets::PointCloudHandle handle{};
            std::vector<math::vec3> positions;
            geometry::Aabb bounds{};
#if ENGINE_RENDERING_HAS_GLAD
            unsigned int vertex_array{0};
            unsigned int position_buffer{0};
            bool gpu_uploaded{false};
#else
            bool gpu_uploaded{false};
#endif
        };

        [[nodiscard]] const PointCloudRecord*
        point_cloud(const assets::PointCloudHandle& handle) const noexcept;
        [[nodiscard]] std::size_t loaded_point_cloud_count() const noexcept;

        struct GraphRecord
        {
            assets::GraphHandle handle{};
            std::vector<math::vec3> positions;
            std::vector<std::uint32_t> indices;
            geometry::Aabb bounds{};
#if ENGINE_RENDERING_HAS_GLAD
            unsigned int vertex_array{0};
            unsigned int position_buffer{0};
            unsigned int index_buffer{0};
            bool gpu_uploaded{false};
#else
            bool gpu_uploaded{false};
#endif
        };

        [[nodiscard]] const GraphRecord* graph(const assets::GraphHandle& handle) const noexcept;

    private:
        MeshResolver mesh_resolver_;
        PointCloudResolver point_cloud_resolver_;
        GraphResolver graph_resolver_;
        std::unordered_map<std::string, MeshRecord> meshes_{};
        std::unordered_map<std::string, GraphRecord> graphs_{};
        std::unordered_map<std::string, PointCloudRecord> point_clouds_{};
        std::unordered_set<std::string> materials_{};
        std::unordered_set<std::string> shaders_{};
        std::size_t mesh_gpu_uploads_{0};
        std::size_t point_cloud_gpu_uploads_{0};

        class AsyncMeshLoader;
        std::unique_ptr<AsyncMeshLoader> async_mesh_loader_{};

        void ensure_mesh_loaded(const assets::MeshHandle& handle);
        bool schedule_mesh_async(const assets::MeshHandle& handle);
        [[nodiscard]] static geometry::SurfaceMesh
        prepare_surface_mesh(geometry::SurfaceMesh mesh);
        void upload_mesh_to_gpu(MeshRecord& record);
        static void destroy_gpu_resources(MeshRecord& record) noexcept;
        void ensure_point_cloud_loaded(const assets::PointCloudHandle& handle);
        static geometry::PointCloud prepare_point_cloud(geometry::PointCloud cloud);
        void upload_point_cloud_to_gpu(PointCloudRecord& record);
        static void destroy_point_cloud_gpu_resources(PointCloudRecord& record) noexcept;
        void ensure_graph_loaded(const assets::GraphHandle& handle);
        static geometry::Graph prepare_graph(geometry::Graph graph);
        void upload_graph_to_gpu(GraphRecord& record);
        static void destroy_graph_gpu_resources(GraphRecord& record) noexcept;
    };
}
