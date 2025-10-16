#pragma once

#include <cstddef>
#include <cstdint>
#include <limits>
#include <ostream>
#include <string_view>

namespace engine::rendering
{
    struct FrameGraphResourceHandle
    {
        std::size_t index{std::numeric_limits<std::size_t>::max()};

        [[nodiscard]] bool valid() const noexcept
        {
            return index != std::numeric_limits<std::size_t>::max();
        }

        friend bool operator==(FrameGraphResourceHandle lhs, FrameGraphResourceHandle rhs) noexcept
        {
            return lhs.index == rhs.index;
        }
    };

    /// Format expected for a frame-graph resource.
    enum class ResourceFormat
    {
        Unknown,
        Rgba8Unorm,
        Rgba16f,
        Rgba32f,
        Depth24Stencil8,
        Depth32f,
    };

    inline std::ostream& operator<<(std::ostream& os, ResourceFormat format)
    {
        switch (format)
        {
        case ResourceFormat::Unknown:
            return os << "Unknown";
        case ResourceFormat::Rgba8Unorm:
            return os << "Rgba8Unorm";
        case ResourceFormat::Rgba16f:
            return os << "Rgba16f";
        case ResourceFormat::Rgba32f:
            return os << "Rgba32f";
        case ResourceFormat::Depth24Stencil8:
            return os << "Depth24Stencil8";
        case ResourceFormat::Depth32f:
            return os << "Depth32f";
        }
        return os;
    }

    /// Dimensionality of a frame-graph resource.
    enum class ResourceDimension
    {
        Unknown,
        Buffer,
        Texture1D,
        Texture2D,
        Texture3D,
        CubeMap,
    };

    inline std::ostream& operator<<(std::ostream& os, ResourceDimension dimension)
    {
        switch (dimension)
        {
        case ResourceDimension::Unknown:
            return os << "Unknown";
        case ResourceDimension::Buffer:
            return os << "Buffer";
        case ResourceDimension::Texture1D:
            return os << "Texture1D";
        case ResourceDimension::Texture2D:
            return os << "Texture2D";
        case ResourceDimension::Texture3D:
            return os << "Texture3D";
        case ResourceDimension::CubeMap:
            return os << "CubeMap";
        }
        return os;
    }

    /// Usage mask describing how a frame-graph resource will be accessed.
    enum class ResourceUsage : std::uint32_t
    {
        None = 0,
        TransferSource = 1U << 0U,
        TransferDestination = 1U << 1U,
        ShaderRead = 1U << 2U,
        ShaderWrite = 1U << 3U,
        ColorAttachment = 1U << 4U,
        DepthStencilAttachment = 1U << 5U,
        Present = 1U << 6U,
    };

    inline ResourceUsage operator|(ResourceUsage lhs, ResourceUsage rhs) noexcept
    {
        return static_cast<ResourceUsage>(static_cast<std::uint32_t>(lhs) |
                                          static_cast<std::uint32_t>(rhs));
    }

    inline ResourceUsage& operator|=(ResourceUsage& lhs, ResourceUsage rhs) noexcept
    {
        lhs = lhs | rhs;
        return lhs;
    }

    inline ResourceUsage operator&(ResourceUsage lhs, ResourceUsage rhs) noexcept
    {
        return static_cast<ResourceUsage>(static_cast<std::uint32_t>(lhs) &
                                          static_cast<std::uint32_t>(rhs));
    }

    inline bool any(ResourceUsage usage) noexcept
    {
        return usage != ResourceUsage::None;
    }

    inline bool has_flag(ResourceUsage usage, ResourceUsage flag) noexcept
    {
        return static_cast<std::uint32_t>(usage & flag) != 0U;
    }

    inline std::ostream& operator<<(std::ostream& os, ResourceUsage usage)
    {
        if (!any(usage))
        {
            return os << "None";
        }

        bool first = true;
        const auto append = [&](std::string_view label) {
            if (!first)
            {
                os << '|';
            }
            first = false;
            os << label;
        };

        if (has_flag(usage, ResourceUsage::TransferSource))
        {
            append("TransferSrc");
        }
        if (has_flag(usage, ResourceUsage::TransferDestination))
        {
            append("TransferDst");
        }
        if (has_flag(usage, ResourceUsage::ShaderRead))
        {
            append("ShaderRead");
        }
        if (has_flag(usage, ResourceUsage::ShaderWrite))
        {
            append("ShaderWrite");
        }
        if (has_flag(usage, ResourceUsage::ColorAttachment))
        {
            append("ColorAttachment");
        }
        if (has_flag(usage, ResourceUsage::DepthStencilAttachment))
        {
            append("DepthStencil");
        }
        if (has_flag(usage, ResourceUsage::Present))
        {
            append("Present");
        }
        return os;
    }

    /// State expected before or after a pass executes.
    enum class ResourceState
    {
        Undefined,
        CommonRead,
        CommonWrite,
        ShaderRead,
        ShaderWrite,
        ColorAttachment,
        DepthStencilAttachment,
        CopySource,
        CopyDestination,
        Present,
    };

    inline std::ostream& operator<<(std::ostream& os, ResourceState state)
    {
        switch (state)
        {
        case ResourceState::Undefined:
            return os << "Undefined";
        case ResourceState::CommonRead:
            return os << "CommonRead";
        case ResourceState::CommonWrite:
            return os << "CommonWrite";
        case ResourceState::ShaderRead:
            return os << "ShaderRead";
        case ResourceState::ShaderWrite:
            return os << "ShaderWrite";
        case ResourceState::ColorAttachment:
            return os << "ColorAttachment";
        case ResourceState::DepthStencilAttachment:
            return os << "DepthStencilAttachment";
        case ResourceState::CopySource:
            return os << "CopySource";
        case ResourceState::CopyDestination:
            return os << "CopyDestination";
        case ResourceState::Present:
            return os << "Present";
        }
        return os;
    }
}  // namespace engine::rendering
