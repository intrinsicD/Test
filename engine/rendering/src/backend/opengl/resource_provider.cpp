#include "engine/rendering/backend/opengl/resource_provider.hpp"

#include <algorithm>
#include <cassert>
#include <optional>
#include <stdexcept>
#include <utility>

#ifndef ENGINE_RENDERING_HAS_GLAD
#    define ENGINE_RENDERING_HAS_GLAD 0
#endif

#if ENGINE_RENDERING_HAS_GLAD
#    include <glad/gl.h>
#endif

#include "engine/rendering/frame_graph.hpp"

namespace engine::rendering::backend::opengl
{
    namespace
    {
        [[nodiscard]] resources::QueueNativeHandle make_queue_handle(QueueType queue, std::uint64_t id) noexcept
        {
            resources::QueueNativeHandle native{};
            native.api = resources::GraphicsApi::OpenGL;
            native.queue = queue;
            native.value = id;
            return native;
        }

        [[nodiscard]] resources::CommandBufferNativeHandle make_command_buffer_handle(
            QueueType queue, CommandBufferHandle handle, OpenGLCommandBuffer& buffer, std::uint64_t id)
        {
            resources::CommandBufferNativeHandle native{};
            native.api = resources::GraphicsApi::OpenGL;
            native.queue = queue;
            native.value = reinterpret_cast<std::uintptr_t>(&buffer); // NOLINT
            native.label = std::string{buffer.label()};
            native.index = handle.index;
            if (native.label.empty())
            {
                native.label = "CommandBuffer";
            }
            if (native.value == 0U)
            {
                native.value = static_cast<std::uintptr_t>(id);
            }
            return native;
        }
    } // namespace

    namespace
    {
        [[nodiscard]] bool uses_multisampling(const FrameGraphResourceInfo& info) noexcept
        {
            return info.sample_count != ResourceSampleCount::Count1;
        }

        [[nodiscard]] std::uint32_t sample_count_value(ResourceSampleCount count) noexcept
        {
            switch (count)
            {
            case ResourceSampleCount::Count1:
                return 1U;
            case ResourceSampleCount::Count2:
                return 2U;
            case ResourceSampleCount::Count4:
                return 4U;
            case ResourceSampleCount::Count8:
                return 8U;
            case ResourceSampleCount::Count16:
                return 16U;
            default:
                assert(false && "Unsupported ResourceSampleCount value");
                return 1U;
            }
        }

        struct TextureFormatInfo
        {
#if ENGINE_RENDERING_HAS_GLAD
            GLenum target{GL_TEXTURE_2D};
            GLenum internal_format{GL_RGBA8};
            GLenum format{GL_RGBA};
            GLenum type{GL_UNSIGNED_BYTE};
#endif
            bool depth_attachment{false};
        };

        [[nodiscard]] bool is_texture_resource(const FrameGraphResourceInfo& info) noexcept
        {
            return info.dimension == ResourceDimension::Texture2D
                || info.dimension == ResourceDimension::Texture3D
                || info.dimension == ResourceDimension::Texture1D
                || info.dimension == ResourceDimension::CubeMap;
        }

        [[nodiscard]] bool is_buffer_resource(const FrameGraphResourceInfo& info) noexcept
        {
            return info.dimension == ResourceDimension::Buffer;
        }

        [[nodiscard]] std::uint32_t bytes_per_pixel(ResourceFormat format) noexcept
        {
            switch (format)
            {
            case ResourceFormat::Rgba8Unorm:
                return 4U;
            case ResourceFormat::Rgba16f:
                return 8U;
            case ResourceFormat::Rgba32f:
                return 16U;
            case ResourceFormat::Depth24Stencil8:
                return 4U;
            case ResourceFormat::Depth32f:
                return 4U;
            case ResourceFormat::Unknown:
            default:
                return 0U;
            }
        }

        [[nodiscard]] std::uint32_t layers_for_dimension(const FrameGraphResourceInfo& info) noexcept
        {
            if (info.dimension == ResourceDimension::CubeMap)
            {
                return info.array_layers != 0U ? info.array_layers : 6U;
            }
            return info.array_layers != 0U ? info.array_layers : 1U;
        }

        [[nodiscard]] std::uint64_t estimate_texture_byte_size(const FrameGraphResourceInfo& info) noexcept
        {
            const auto bpp = bytes_per_pixel(info.format);
            if (bpp == 0U)
            {
                return 0;
            }

            const auto width = info.width;
            const auto height = info.dimension == ResourceDimension::Texture1D ? 1U : info.height;
            const auto depth = info.dimension == ResourceDimension::Texture3D ? info.depth : 1U;

            if (width == 0U || height == 0U || depth == 0U)
            {
                return 0;
            }

            const auto layers = static_cast<std::uint64_t>(layers_for_dimension(info));
            const auto samples = static_cast<std::uint64_t>(sample_count_value(info.sample_count));
            const auto levels = info.mip_levels == 0U ? 1U : info.mip_levels;

            std::uint64_t total = 0;
            std::uint32_t level_width = width;
            std::uint32_t level_height = height;
            std::uint32_t level_depth = depth;

            for (std::uint32_t level = 0; level < levels; ++level)
            {
                const auto w = static_cast<std::uint64_t>(std::max(1U, level_width));
                const auto h = static_cast<std::uint64_t>(std::max(1U, level_height));
                const auto d = static_cast<std::uint64_t>(std::max(1U, level_depth));
                total += w * h * d * layers * static_cast<std::uint64_t>(bpp) * samples;

                if (level_width > 1U)
                {
                    level_width >>= 1U;
                }
                if (info.dimension != ResourceDimension::Texture1D && level_height > 1U)
                {
                    level_height >>= 1U;
                }
                if (info.dimension == ResourceDimension::Texture3D && level_depth > 1U)
                {
                    level_depth >>= 1U;
                }
            }

            return total;
        }

#if ENGINE_RENDERING_HAS_GLAD
        [[nodiscard]] GLenum buffer_target(ResourceUsage usage) noexcept
        {
            if (rendering::has_flag(usage, ResourceUsage::ShaderWrite))
            {
                return GL_SHADER_STORAGE_BUFFER;
            }
            if (rendering::has_flag(usage, ResourceUsage::ShaderRead))
            {
                return GL_UNIFORM_BUFFER;
            }
            if (rendering::has_flag(usage, ResourceUsage::TransferDestination))
            {
                return GL_COPY_WRITE_BUFFER;
            }
            if (rendering::has_flag(usage, ResourceUsage::TransferSource))
            {
                return GL_COPY_READ_BUFFER;
            }
            return GL_ARRAY_BUFFER;
        }

        [[nodiscard]] GLenum buffer_usage_hint(ResourceUsage usage) noexcept
        {
            if (rendering::has_flag(usage, ResourceUsage::TransferDestination)
                || rendering::has_flag(usage, ResourceUsage::TransferSource)
                || rendering::has_flag(usage, ResourceUsage::ShaderWrite))
            {
                return GL_DYNAMIC_DRAW;
            }
            return GL_STATIC_DRAW;
        }

        [[nodiscard]] GLenum texture_target(const FrameGraphResourceInfo& info) noexcept
        {
            if (uses_multisampling(info))
            {
                if (info.dimension == ResourceDimension::Texture2D && info.array_layers > 1U)
                {
                    return GL_TEXTURE_2D_MULTISAMPLE_ARRAY;
                }
                if (info.dimension == ResourceDimension::Texture2D)
                {
                    return GL_TEXTURE_2D_MULTISAMPLE;
                }
            }

            switch (info.dimension)
            {
            case ResourceDimension::Texture1D:
                return GL_TEXTURE_1D;
            case ResourceDimension::Texture3D:
                return GL_TEXTURE_3D;
            case ResourceDimension::CubeMap:
                return GL_TEXTURE_CUBE_MAP;
            case ResourceDimension::Texture2D:
            default:
                return GL_TEXTURE_2D;
            }
        }

        [[nodiscard]] std::optional<TextureFormatInfo> translate_texture_format(
            const FrameGraphResourceInfo& info) noexcept
        {
            TextureFormatInfo translated{};
            translated.target = texture_target(info);
            switch (info.format)
            {
            case ResourceFormat::Rgba8Unorm:
                translated.internal_format = GL_RGBA8;
                translated.format = GL_RGBA;
                translated.type = GL_UNSIGNED_BYTE;
                translated.depth_attachment = false;
                return translated;
            case ResourceFormat::Rgba16f:
                translated.internal_format = GL_RGBA16F;
                translated.format = GL_RGBA;
                translated.type = GL_HALF_FLOAT;
                translated.depth_attachment = false;
                return translated;
            case ResourceFormat::Rgba32f:
                translated.internal_format = GL_RGBA32F;
                translated.format = GL_RGBA;
                translated.type = GL_FLOAT;
                translated.depth_attachment = false;
                return translated;
            case ResourceFormat::Depth24Stencil8:
                translated.internal_format = GL_DEPTH24_STENCIL8;
                translated.format = GL_DEPTH_STENCIL;
                translated.type = GL_UNSIGNED_INT_24_8;
                translated.depth_attachment = true;
                return translated;
            case ResourceFormat::Depth32f:
                translated.internal_format = GL_DEPTH_COMPONENT32F;
                translated.format = GL_DEPTH_COMPONENT;
                translated.type = GL_FLOAT;
                translated.depth_attachment = true;
                return translated;
            case ResourceFormat::Unknown:
                break;
            }
            return std::nullopt;
        }

        [[nodiscard]] GLuint create_buffer(const FrameGraphResourceInfo& info, GLenum target)
        {
            GLuint name = 0;
            if (glad_glCreateBuffers != nullptr)
            {
                glad_glCreateBuffers(1, &name);
            }
            else if (glad_glGenBuffers != nullptr)
            {
                glad_glGenBuffers(1, &name);
            }

            if (name == 0)
            {
                return 0;
            }

            if (info.size_bytes != 0)
            {
                const auto usage = buffer_usage_hint(info.usage);
                if (glad_glNamedBufferData != nullptr)
                {
                    glad_glNamedBufferData(name, static_cast<GLsizeiptr>(info.size_bytes), nullptr, usage);
                }
                else if (glad_glBindBuffer != nullptr && glad_glBufferData != nullptr)
                {
                    glad_glBindBuffer(target, name);
                    glad_glBufferData(target, static_cast<GLsizeiptr>(info.size_bytes), nullptr, usage);
                    glad_glBindBuffer(target, 0);
                }
            }

            return name;
        }

        [[nodiscard]] GLuint create_texture(const FrameGraphResourceInfo& info,
                                            const TextureFormatInfo& format)
        {
            GLuint name = 0;
            if (glad_glCreateTextures != nullptr)
            {
                glad_glCreateTextures(format.target, 1, &name);
            }
            else if (glad_glGenTextures != nullptr)
            {
                glad_glGenTextures(1, &name);
            }

            if (name == 0)
            {
                return 0;
            }

            const auto bind_texture = [&]()
            {
                if (glad_glBindTexture != nullptr)
                {
                    glad_glBindTexture(format.target, name);
                }
            };

            const auto unbind_texture = [&]()
            {
                if (glad_glBindTexture != nullptr)
                {
                    glad_glBindTexture(format.target, 0);
                }
            };

            const auto multisampled = uses_multisampling(info);
            if (multisampled)
            {
                const auto samples = static_cast<GLsizei>(sample_count_value(info.sample_count));
                if (format.target == GL_TEXTURE_2D_MULTISAMPLE)
                {
                    if (glad_glTexImage2DMultisample != nullptr)
                    {
                        bind_texture();
                        glad_glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, samples,
                                                     static_cast<GLint>(format.internal_format),
                                                     static_cast<GLsizei>(info.width),
                                                     static_cast<GLsizei>(info.height), GL_TRUE);
                        unbind_texture();
                    }
                    else
                    {
                        destroy_gl_texture(name);
                        return 0;
                    }
                }
                else if (format.target == GL_TEXTURE_2D_MULTISAMPLE_ARRAY)
                {
                    if (glad_glTexImage3DMultisample != nullptr)
                    {
                        bind_texture();
                        glad_glTexImage3DMultisample(GL_TEXTURE_2D_MULTISAMPLE_ARRAY, samples,
                                                     static_cast<GLint>(format.internal_format),
                                                     static_cast<GLsizei>(info.width),
                                                     static_cast<GLsizei>(info.height),
                                                     static_cast<GLsizei>(info.array_layers), GL_TRUE);
                        unbind_texture();
                    }
                    else
                    {
                        destroy_gl_texture(name);
                        return 0;
                    }
                }
                else
                {
                    destroy_gl_texture(name);
                    return 0;
                }

                return name;
            }

            bind_texture();

            if (format.target == GL_TEXTURE_1D)
            {
                if (glad_glTexImage1D != nullptr)
                {
                    glad_glTexImage1D(GL_TEXTURE_1D, 0, static_cast<GLint>(format.internal_format),
                                      static_cast<GLsizei>(info.width), 0, format.format, format.type, nullptr);
                }
            }
            else if (format.target == GL_TEXTURE_3D)
            {
                if (glad_glTexImage3D != nullptr)
                {
                    glad_glTexImage3D(GL_TEXTURE_3D, 0, static_cast<GLint>(format.internal_format),
                                      static_cast<GLsizei>(info.width), static_cast<GLsizei>(info.height),
                                      static_cast<GLsizei>(info.depth), 0, format.format, format.type, nullptr);
                }
            }
            else
            {
                if (glad_glTexImage2D != nullptr)
                {
                    glad_glTexImage2D(format.target, 0, static_cast<GLint>(format.internal_format),
                                      static_cast<GLsizei>(info.width), static_cast<GLsizei>(info.height), 0,
                                      format.format, format.type, nullptr);
                }
            }

            if (glad_glTexParameteri != nullptr)
            {
                glad_glTexParameteri(format.target, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                glad_glTexParameteri(format.target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                glad_glTexParameteri(format.target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                glad_glTexParameteri(format.target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            }

            unbind_texture();

            return name;
        }

        void destroy_gl_texture(GLuint name)
        {
            if (name != 0 && glad_glDeleteTextures != nullptr)
            {
                glad_glDeleteTextures(1, &name);
            }
        }

        void destroy_gl_buffer(GLuint name)
        {
            if (name != 0 && glad_glDeleteBuffers != nullptr)
            {
                glad_glDeleteBuffers(1, &name);
            }
        }
#endif  // ENGINE_RENDERING_HAS_GLAD
    } // namespace

    OpenGLGpuResourceProvider::OpenGLGpuResourceProvider(std::uint64_t retention_frames)
        : retention_frames_{retention_frames}
    {
    }

    OpenGLGpuResourceProvider::~OpenGLGpuResourceProvider()
    {
        for (auto& [index, record] : buffers_)
        {
            static_cast<void>(index);
            destroy_buffer(record);
        }
        for (auto& [index, record] : textures_)
        {
            static_cast<void>(index);
            destroy_texture(record);
        }
    }

    resources::GraphicsApi OpenGLGpuResourceProvider::api() const noexcept
    {
        return resources::GraphicsApi::OpenGL;
    }

    void OpenGLGpuResourceProvider::begin_frame()
    {
        ++current_frame_;
        acquired_.clear();
        released_.clear();
    }

    void OpenGLGpuResourceProvider::end_frame()
    {
        const auto should_collect_record = [this](const auto& record) {
            if (record.in_use)
            {
                return false;
            }
            if (record.last_used_frame == 0)
            {
                return false;
            }
            if (current_frame_ <= record.last_used_frame)
            {
                return false;
            }
            return (current_frame_ - record.last_used_frame) > retention_frames_;
        };

        std::erase_if(buffers_, [&](auto& entry) {
            if (should_collect_record(entry.second))
            {
                destroy_buffer(entry.second);
                return true;
            }
            return false;
        });

        std::erase_if(textures_, [&](auto& entry) {
            if (should_collect_record(entry.second))
            {
                destroy_texture(entry.second);
                return true;
            }
            return false;
        });
    }

    void OpenGLGpuResourceProvider::set_retention_frames(std::uint64_t frames) noexcept
    {
        retention_frames_ = frames;
    }

    resources::QueueNativeHandle OpenGLGpuResourceProvider::queue_handle(QueueType queue) const
    {
        auto it = queues_.find(queue);
        if (it != queues_.end())
        {
            return it->second.native;
        }

        QueueRecord record{};
        record.native = make_queue_handle(queue, next_queue_id_++);
        auto [inserted, success] = queues_.emplace(queue, record);
        static_cast<void>(success);
        return inserted->second.native;
    }

    resources::CommandBufferNativeHandle OpenGLGpuResourceProvider::allocate_command_buffer(
        QueueType queue, std::string_view label, CommandBufferHandle handle)
    {
        auto& record = command_buffers_[handle.index];
        if (record.buffer == nullptr)
        {
            record.buffer = std::make_unique<OpenGLCommandBuffer>();
        }

        record.buffer->reset(label, handle, queue);
        return make_command_buffer_handle(queue, handle, *record.buffer, next_command_buffer_id_++);
    }

    void OpenGLGpuResourceProvider::recycle_command_buffer(CommandBufferHandle handle)
    {
        auto it = command_buffers_.find(handle.index);
        if (it == command_buffers_.end())
        {
            return;
        }
        if (it->second.buffer != nullptr)
        {
            it->second.buffer->clear_commands();
        }
    }

    resources::FenceNativeHandle OpenGLGpuResourceProvider::resolve_fence(const resources::Fence& fence)
    {
        const auto name = std::string{fence.name()};
        auto& record = fences_[name];
        if (record.native.value == 0U)
        {
            record.native.api = resources::GraphicsApi::OpenGL;
            record.native.value = next_fence_id_++;
        }
        return record.native;
    }

    resources::TimelineSemaphoreNativeHandle OpenGLGpuResourceProvider::resolve_semaphore(
        const resources::TimelineSemaphore& semaphore)
    {
        const auto name = std::string{semaphore.name()};
        auto& record = timelines_[name];
        if (record.native.value == 0U)
        {
            record.native.api = resources::GraphicsApi::OpenGL;
            record.native.value = next_timeline_id_++;
        }
        return record.native;
    }

    void OpenGLGpuResourceProvider::on_transient_acquire(FrameGraphResourceHandle handle,
                                                         const FrameGraphResourceInfo& info)
    {
        if (!handle.valid())
        {
            return;
        }

        acquired_.push_back(ResourceEventRecord{handle, info});

        if (is_buffer_resource(info))
        {
            auto it = buffers_.find(handle.index);
            if (it == buffers_.end())
            {
                allocate_buffer(handle.index, info);
                it = buffers_.find(handle.index);
            }
            else if (!buffer_descriptor_matches(it->second, info))
            {
                destroy_buffer(it->second);
                allocate_buffer(handle.index, info);
                it = buffers_.find(handle.index);
            }

            if (it != buffers_.end())
            {
                it->second.in_use = true;
                it->second.last_used_frame = current_frame_;
            }

            if (!is_texture_resource(info))
            {
                return;
            }
        }

        if (!is_texture_resource(info))
        {
            return;
        }

        auto it = textures_.find(handle.index);
        if (it == textures_.end())
        {
            allocate_texture(handle.index, info);
            it = textures_.find(handle.index);
        }
        else if (!texture_descriptor_matches(it->second, info))
        {
            destroy_texture(it->second);
            allocate_texture(handle.index, info);
            it = textures_.find(handle.index);
        }

        if (it != textures_.end())
        {
            it->second.in_use = true;
            it->second.last_used_frame = current_frame_;
        }
    }

    void OpenGLGpuResourceProvider::on_transient_release(FrameGraphResourceHandle handle,
                                                         const FrameGraphResourceInfo& info)
    {
        if (!handle.valid())
        {
            return;
        }

        released_.push_back(ResourceEventRecord{handle, info});

        if (is_buffer_resource(info))
        {
            auto buffer_it = buffers_.find(handle.index);
            if (buffer_it != buffers_.end())
            {
                buffer_it->second.in_use = false;
                buffer_it->second.last_used_frame = current_frame_;
            }

            if (!is_texture_resource(info))
            {
                return;
            }
        }

        auto it = textures_.find(handle.index);
        if (it == textures_.end())
        {
            return;
        }

        static_cast<void>(info);
        it->second.in_use = false;
        it->second.last_used_frame = current_frame_;
    }

    OpenGLCommandBuffer* OpenGLGpuResourceProvider::command_buffer(CommandBufferHandle handle) noexcept
    {
        auto it = command_buffers_.find(handle.index);
        if (it == command_buffers_.end())
        {
            return nullptr;
        }
        return it->second.buffer.get();
    }

    const OpenGLCommandBuffer* OpenGLGpuResourceProvider::command_buffer(CommandBufferHandle handle) const noexcept
    {
        auto it = command_buffers_.find(handle.index);
        if (it == command_buffers_.end())
        {
            return nullptr;
        }
        return it->second.buffer.get();
    }

    const std::vector<OpenGLGpuResourceProvider::ResourceEventRecord>&
    OpenGLGpuResourceProvider::acquired() const noexcept
    {
        return acquired_;
    }

    const std::vector<OpenGLGpuResourceProvider::ResourceEventRecord>&
    OpenGLGpuResourceProvider::released() const noexcept
    {
        return released_;
    }

    const OpenGLGpuResourceProvider::BufferRecord*
    OpenGLGpuResourceProvider::buffer(FrameGraphResourceHandle handle) const noexcept
    {
        if (!handle.valid())
        {
            return nullptr;
        }

        auto it = buffers_.find(handle.index);
        if (it == buffers_.end())
        {
            return nullptr;
        }
        return &it->second;
    }

    const OpenGLGpuResourceProvider::TextureRecord*
    OpenGLGpuResourceProvider::texture(FrameGraphResourceHandle handle) const noexcept
    {
        if (!handle.valid())
        {
            return nullptr;
        }

        auto it = textures_.find(handle.index);
        if (it == textures_.end())
        {
            return nullptr;
        }
        return &it->second;
    }

    void OpenGLGpuResourceProvider::destroy_buffer(BufferRecord& record) noexcept
    {
#if ENGINE_RENDERING_HAS_GLAD
        if (record.native_allocation)
        {
            destroy_gl_buffer(static_cast<GLuint>(record.handle));
        }
#endif
        if (buffer_bytes_ >= record.size_bytes)
        {
            buffer_bytes_ -= record.size_bytes;
        }
        else
        {
            buffer_bytes_ = 0;
        }
        record.handle = 0;
        record.target = 0;
        record.native_allocation = false;
        record.in_use = false;
        record.size_bytes = 0;
    }

    void OpenGLGpuResourceProvider::destroy_texture(TextureRecord& record) noexcept
    {
#if ENGINE_RENDERING_HAS_GLAD
        if (record.native_allocation)
        {
            destroy_gl_texture(static_cast<GLuint>(record.handle));
        }
#endif
        if (texture_bytes_ >= record.byte_size)
        {
            texture_bytes_ -= record.byte_size;
        }
        else
        {
            texture_bytes_ = 0;
        }
        record.handle = 0;
        record.in_use = false;
        record.depth_attachment = false;
        record.target = 0;
        record.multisampled = false;
        record.native_allocation = false;
        record.byte_size = 0;
    }

    void OpenGLGpuResourceProvider::allocate_buffer(std::size_t index, const FrameGraphResourceInfo& info)
    {
        BufferRecord record{};
        record.name = std::string{info.name};
        record.usage = info.usage;
        record.size_bytes = info.size_bytes;
        record.in_use = true;
        record.last_used_frame = current_frame_;

        if (record.size_bytes != 0)
        {
            buffer_bytes_ += record.size_bytes;
        }

#if ENGINE_RENDERING_HAS_GLAD
        const auto target = buffer_target(info.usage);
        const auto gl_name = create_buffer(info, target);
        if (gl_name != 0)
        {
            record.handle = gl_name;
            record.target = target;
            record.native_allocation = true;
        }
#endif

        if (record.handle == 0)
        {
            record.handle = next_buffer_id_++;
            record.target = 0;
            record.native_allocation = false;
        }

        buffers_.insert_or_assign(index, record);
    }

    void OpenGLGpuResourceProvider::allocate_texture(std::size_t index, const FrameGraphResourceInfo& info)
    {
        TextureRecord record{};
        record.name = std::string{info.name};
        record.format = info.format;
        record.usage = info.usage;
        record.dimension = info.dimension;
        record.sample_count = info.sample_count;
        record.width = info.width;
        record.height = info.height;
        record.depth = info.depth;
        record.array_layers = info.array_layers;
        record.mip_levels = info.mip_levels;
        record.multisampled = uses_multisampling(info);
        record.in_use = true;
        record.last_used_frame = current_frame_;
        record.byte_size = estimate_texture_byte_size(info);

        if (record.byte_size != 0)
        {
            texture_bytes_ += record.byte_size;
        }

#if ENGINE_RENDERING_HAS_GLAD
        if (const auto translated = translate_texture_format(info))
        {
            const auto gl_name = create_texture(info, *translated);
            if (gl_name != 0)
            {
                record.handle = gl_name;
                record.target = translated->target;
                record.native_allocation = true;
                record.depth_attachment = translated->depth_attachment;
            }
        }
#endif

        if (record.handle == 0)
        {
            record.handle = next_texture_id_++;
            record.native_allocation = false;
            record.target = 0;
            record.depth_attachment = info.format == ResourceFormat::Depth24Stencil8
                || info.format == ResourceFormat::Depth32f;
        }

        textures_.insert_or_assign(index, record);
    }

    bool OpenGLGpuResourceProvider::buffer_descriptor_matches(const BufferRecord& record,
                                                              const FrameGraphResourceInfo& info) const noexcept
    {
        return record.usage == info.usage && record.size_bytes == info.size_bytes;
    }

    bool OpenGLGpuResourceProvider::texture_descriptor_matches(const TextureRecord& record,
                                                               const FrameGraphResourceInfo& info) const noexcept
    {
        return record.format == info.format && record.usage == info.usage && record.dimension == info.dimension
            && record.sample_count == info.sample_count && record.width == info.width
            && record.height == info.height && record.depth == info.depth
            && record.array_layers == info.array_layers && record.mip_levels == info.mip_levels;
    }
}