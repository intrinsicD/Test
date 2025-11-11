#include "engine/rendering/backend/opengl/render_resource_provider.hpp"

#include <span>
#include <stdexcept>
#include <utility>

#if ENGINE_RENDERING_HAS_GLAD
#    include <glad/gl.h>
#endif

#include "engine/geometry/api.hpp"
#include "engine/geometry/point_cloud/point_cloud.hpp"

namespace engine::rendering::backend::opengl
{
    namespace
    {
        [[nodiscard]] bool identifier_empty(std::string_view id) noexcept
        {
            return id.empty();
        }

        template <typename Handle>
        void record_handle(std::unordered_set<std::string>& storage, const Handle& handle)
        {
            if (handle.empty())
            {
                return;
            }
            const auto& id = handle.id();
            if (identifier_empty(id))
            {
                return;
            }
            storage.insert(id);
        }
    } // namespace

    OpenGLRenderResourceProvider::OpenGLRenderResourceProvider(MeshResolver mesh_resolver,
                                                               PointCloudResolver point_cloud_resolver)
        : mesh_resolver_(std::move(mesh_resolver))
        , point_cloud_resolver_(std::move(point_cloud_resolver))
    {
        if (!mesh_resolver_)
        {
            throw std::invalid_argument(
                "OpenGLRenderResourceProvider requires a mesh resolver callback");
        }
    }

    OpenGLRenderResourceProvider::~OpenGLRenderResourceProvider()
    {
        for (auto& [_, record] : meshes_)
        {
            destroy_gpu_resources(record);
        }

        for (auto& [_, record] : point_clouds_)
        {
            destroy_point_cloud_gpu_resources(record);
        }
    }

    void OpenGLRenderResourceProvider::require_mesh(const assets::MeshHandle& handle)
    {
        if (handle.empty())
        {
            return;
        }
        ensure_mesh_loaded(handle);
    }

    void OpenGLRenderResourceProvider::require_graph(const assets::GraphHandle& handle)
    {
        record_handle(graphs_, handle);
    }

    void OpenGLRenderResourceProvider::require_point_cloud(const assets::PointCloudHandle& handle)
    {
        if (handle.empty())
        {
            return;
        }
        ensure_point_cloud_loaded(handle);
    }

    void OpenGLRenderResourceProvider::require_material(const assets::MaterialHandle& handle)
    {
        record_handle(materials_, handle);
    }

    void OpenGLRenderResourceProvider::require_shader(const assets::ShaderHandle& handle)
    {
        record_handle(shaders_, handle);
    }

    const OpenGLRenderResourceProvider::MeshRecord*
    OpenGLRenderResourceProvider::mesh(const assets::MeshHandle& handle) const noexcept
    {
        if (handle.empty())
        {
            return nullptr;
        }
        const auto& id = handle.id();
        if (identifier_empty(id))
        {
            return nullptr;
        }
        if (const auto it = meshes_.find(id); it != meshes_.end())
        {
            return &it->second;
        }
        return nullptr;
    }

    std::size_t OpenGLRenderResourceProvider::loaded_mesh_count() const noexcept
    {
        return meshes_.size();
    }

    std::size_t OpenGLRenderResourceProvider::mesh_gpu_upload_count() const noexcept
    {
        return mesh_gpu_uploads_;
    }

    const OpenGLRenderResourceProvider::PointCloudRecord*
    OpenGLRenderResourceProvider::point_cloud(const assets::PointCloudHandle& handle) const noexcept
    {
        if (handle.empty())
        {
            return nullptr;
        }
        const auto& id = handle.id();
        if (identifier_empty(id))
        {
            return nullptr;
        }
        if (const auto it = point_clouds_.find(id); it != point_clouds_.end())
        {
            return &it->second;
        }
        return nullptr;
    }

    std::size_t OpenGLRenderResourceProvider::loaded_point_cloud_count() const noexcept
    {
        return point_clouds_.size();
    }

    void OpenGLRenderResourceProvider::ensure_mesh_loaded(const assets::MeshHandle& handle)
    {
        const auto& id = handle.id();
        if (identifier_empty(id))
        {
            throw std::invalid_argument("Mesh handle identifier cannot be empty");
        }

        if (meshes_.find(id) != meshes_.end())
        {
            return;
        }

        auto resolved = mesh_resolver_(handle);
        if (!resolved.has_value())
        {
            throw std::runtime_error("OpenGLRenderResourceProvider could not resolve mesh: " + id);
        }

        auto surface = prepare_surface_mesh(std::move(*resolved));

        MeshRecord record{};
        record.handle = handle;
        record.positions = std::move(surface.positions);
        record.normals = std::move(surface.normals);
        record.texture_coordinates = std::move(surface.texture_coordinates);
        record.indices = std::move(surface.indices);
        record.bounds = surface.bounds;

        upload_mesh_to_gpu(record);

        meshes_.insert_or_assign(id, std::move(record));
    }

    geometry::SurfaceMesh OpenGLRenderResourceProvider::prepare_surface_mesh(geometry::SurfaceMesh mesh)
    {
        if (mesh.positions.empty())
        {
            throw std::runtime_error("Surface mesh is missing vertex positions");
        }

        if (mesh.indices.empty())
        {
            throw std::runtime_error("Surface mesh is missing indices");
        }

        if (mesh.normals.size() != mesh.positions.size())
        {
            mesh.normals.clear();
        }
        if (mesh.normals.empty())
        {
            geometry::recompute_vertex_normals(mesh);
        }

        if (mesh.texture_coordinates.size() != mesh.positions.size())
        {
            mesh.texture_coordinates.clear();
        }

        geometry::update_bounds(mesh);
        return mesh;
    }

    void OpenGLRenderResourceProvider::upload_mesh_to_gpu(MeshRecord& record)
    {
#if ENGINE_RENDERING_HAS_GLAD
        const bool has_gl = glad_glGenVertexArrays != nullptr && glad_glBindVertexArray != nullptr
            && glad_glGenBuffers != nullptr && glad_glBindBuffer != nullptr
            && glad_glBufferData != nullptr && glad_glEnableVertexAttribArray != nullptr
            && glad_glVertexAttribPointer != nullptr;

        if (!has_gl || record.positions.empty())
        {
            return;
        }

        glad_glGenVertexArrays(1, &record.vertex_array);
        glad_glBindVertexArray(record.vertex_array);

        glad_glGenBuffers(1, &record.position_buffer);
        glad_glBindBuffer(GL_ARRAY_BUFFER, record.position_buffer);
        glad_glBufferData(GL_ARRAY_BUFFER,
                          static_cast<GLsizeiptr>(record.positions.size() * sizeof(math::vec3)),
                          reinterpret_cast<const void*>(record.positions.data()),
                          GL_STATIC_DRAW);
        glad_glEnableVertexAttribArray(0);
        glad_glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(math::vec3), nullptr);

        if (!record.normals.empty())
        {
            glad_glGenBuffers(1, &record.normal_buffer);
            glad_glBindBuffer(GL_ARRAY_BUFFER, record.normal_buffer);
            glad_glBufferData(GL_ARRAY_BUFFER,
                              static_cast<GLsizeiptr>(record.normals.size() * sizeof(math::vec3)),
                              reinterpret_cast<const void*>(record.normals.data()),
                              GL_STATIC_DRAW);
            glad_glEnableVertexAttribArray(1);
            glad_glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(math::vec3), nullptr);
        }

        if (!record.texture_coordinates.empty())
        {
            glad_glGenBuffers(1, &record.texcoord_buffer);
            glad_glBindBuffer(GL_ARRAY_BUFFER, record.texcoord_buffer);
            glad_glBufferData(GL_ARRAY_BUFFER,
                              static_cast<GLsizeiptr>(record.texture_coordinates.size()
                                                      * sizeof(math::vec2)),
                              reinterpret_cast<const void*>(record.texture_coordinates.data()),
                              GL_STATIC_DRAW);
            glad_glEnableVertexAttribArray(2);
            glad_glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(math::vec2), nullptr);
        }

        if (!record.indices.empty())
        {
            glad_glGenBuffers(1, &record.index_buffer);
            glad_glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, record.index_buffer);
            glad_glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                              static_cast<GLsizeiptr>(record.indices.size() * sizeof(std::uint32_t)),
                              reinterpret_cast<const void*>(record.indices.data()),
                              GL_STATIC_DRAW);
        }

        glad_glBindBuffer(GL_ARRAY_BUFFER, 0);
        glad_glBindVertexArray(0);

        record.gpu_uploaded = true;
        ++mesh_gpu_uploads_;
#else
        static_cast<void>(record);
#endif
    }

    void OpenGLRenderResourceProvider::destroy_gpu_resources(MeshRecord& record) noexcept
    {
#if ENGINE_RENDERING_HAS_GLAD
        if (record.index_buffer != 0U && glad_glDeleteBuffers != nullptr)
        {
            glad_glDeleteBuffers(1, &record.index_buffer);
            record.index_buffer = 0U;
        }
        if (record.texcoord_buffer != 0U && glad_glDeleteBuffers != nullptr)
        {
            glad_glDeleteBuffers(1, &record.texcoord_buffer);
            record.texcoord_buffer = 0U;
        }
        if (record.normal_buffer != 0U && glad_glDeleteBuffers != nullptr)
        {
            glad_glDeleteBuffers(1, &record.normal_buffer);
            record.normal_buffer = 0U;
        }
        if (record.position_buffer != 0U && glad_glDeleteBuffers != nullptr)
        {
            glad_glDeleteBuffers(1, &record.position_buffer);
            record.position_buffer = 0U;
        }
        if (record.vertex_array != 0U && glad_glDeleteVertexArrays != nullptr)
        {
            glad_glDeleteVertexArrays(1, &record.vertex_array);
            record.vertex_array = 0U;
        }
        record.gpu_uploaded = false;
#else
        static_cast<void>(record);
#endif
    }

    void OpenGLRenderResourceProvider::ensure_point_cloud_loaded(const assets::PointCloudHandle& handle)
    {
        const auto& id = handle.id();
        if (identifier_empty(id))
        {
            throw std::invalid_argument("Point cloud handle identifier cannot be empty");
        }

        if (point_clouds_.find(id) != point_clouds_.end())
        {
            return;
        }

        if (!point_cloud_resolver_)
        {
            throw std::runtime_error("OpenGLRenderResourceProvider requires a point cloud resolver to load point clouds");
        }

        auto resolved = point_cloud_resolver_(handle);
        if (!resolved.has_value())
        {
            throw std::runtime_error("OpenGLRenderResourceProvider could not resolve point cloud: " + id);
        }

        auto cloud = prepare_point_cloud(std::move(*resolved));

        PointCloudRecord record{};
        record.handle = handle;
        auto positions = cloud.interface.positions();
        record.positions.assign(positions.begin(), positions.end());
        record.bounds = geometry::BoundingAabb(std::span{record.positions});

        upload_point_cloud_to_gpu(record);

        point_clouds_.insert_or_assign(id, std::move(record));
    }

    geometry::PointCloud OpenGLRenderResourceProvider::prepare_point_cloud(geometry::PointCloud cloud)
    {
        if (cloud.interface.vertex_count() == 0U)
        {
            throw std::runtime_error("Point cloud is missing vertices");
        }
        return cloud;
    }

    void OpenGLRenderResourceProvider::upload_point_cloud_to_gpu(PointCloudRecord& record)
    {
#if ENGINE_RENDERING_HAS_GLAD
        const bool has_gl = glad_glGenVertexArrays != nullptr && glad_glBindVertexArray != nullptr
            && glad_glGenBuffers != nullptr && glad_glBindBuffer != nullptr && glad_glBufferData != nullptr
            && glad_glEnableVertexAttribArray != nullptr && glad_glVertexAttribPointer != nullptr;

        if (!has_gl || record.positions.empty())
        {
            return;
        }

        glad_glGenVertexArrays(1, &record.vertex_array);
        glad_glBindVertexArray(record.vertex_array);

        glad_glGenBuffers(1, &record.position_buffer);
        glad_glBindBuffer(GL_ARRAY_BUFFER, record.position_buffer);
        glad_glBufferData(GL_ARRAY_BUFFER,
                          static_cast<GLsizeiptr>(record.positions.size() * sizeof(math::vec3)),
                          reinterpret_cast<const void*>(record.positions.data()),
                          GL_STATIC_DRAW);
        glad_glEnableVertexAttribArray(0);
        glad_glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(math::vec3), nullptr);

        glad_glBindVertexArray(0);

        record.gpu_uploaded = true;
        point_cloud_gpu_uploads_ += 1U;
#else
        static_cast<void>(record);
#endif
    }

    void OpenGLRenderResourceProvider::destroy_point_cloud_gpu_resources(PointCloudRecord& record) noexcept
    {
#if ENGINE_RENDERING_HAS_GLAD
        if (record.position_buffer != 0U && glad_glDeleteBuffers != nullptr)
        {
            glad_glDeleteBuffers(1, &record.position_buffer);
            record.position_buffer = 0U;
        }
        if (record.vertex_array != 0U && glad_glDeleteVertexArrays != nullptr)
        {
            glad_glDeleteVertexArrays(1, &record.vertex_array);
            record.vertex_array = 0U;
        }
#else
        static_cast<void>(record);
#endif
        record.gpu_uploaded = false;
    }
}
