#pragma once

#include "engine/assets/async.hpp"
#include "engine/assets/texture_asset.hpp"

#include "engine/core/diagnostics/result.hpp"

#include <cstddef>
#include <filesystem>
#include <memory>
#include <mutex>
#include <span>
#include <vector>

namespace engine::assets
{
    struct TextureDecodedImage
    {
        TextureFormat format{TextureFormat::unknown};
        TextureDimensions extent{};
        std::uint32_t channel_count{0};
        std::vector<std::byte> pixels{};
        bool high_dynamic_range{false};
    };

    class ITextureDecoder
    {
    public:
        virtual ~ITextureDecoder() = default;

        [[nodiscard]] virtual bool supports(const TextureAssetDescriptor& descriptor,
                                            std::span<const std::byte> encoded) const noexcept = 0;

        [[nodiscard]] virtual engine::Result<TextureDecodedImage, AssetLoadError> decode(
            const TextureAssetDescriptor& descriptor,
            std::span<const std::byte> encoded) const = 0;
    };

    class TextureDecoderRegistry
    {
    public:
        static TextureDecoderRegistry& instance();

        void register_decoder(std::unique_ptr<ITextureDecoder> decoder);

        [[nodiscard]] engine::Result<TextureDecodedImage, AssetLoadError> decode(
            const TextureAssetDescriptor& descriptor,
            std::span<const std::byte> encoded) const;

    private:
        TextureDecoderRegistry();

        TextureDecoderRegistry(const TextureDecoderRegistry&) = delete;
        TextureDecoderRegistry& operator=(const TextureDecoderRegistry&) = delete;

        std::vector<std::unique_ptr<ITextureDecoder>> decoders_{};
        mutable std::mutex mutex_{};
    };
} // namespace engine::assets
