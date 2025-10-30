#include "engine/assets/texture_decoder.hpp"

#include "engine/assets/async.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <cstring>
#include <cstdlib>
#include <filesystem>
#include <span>
#include <string>
#include <string_view>
#include <utility>

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_STATIC
#define STBI_NO_THREAD_LOCALS
#define STBI_MALLOC(sz) std::malloc(sz)
#define STBI_REALLOC(p,sz) std::realloc(p,sz)
#define STBI_FREE(p) std::free(p)
#include <stb_image.h>

namespace engine::assets
{
    namespace
    {
        [[nodiscard]] std::string normalize_extension(const std::filesystem::path& path)
        {
            std::string extension = path.extension().string();
            std::transform(extension.begin(), extension.end(), extension.begin(),
                           [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
            return extension;
        }

        class StbTextureDecoder final : public ITextureDecoder
        {
        public:
            [[nodiscard]] bool supports(const TextureAssetDescriptor& descriptor,
                                        std::span<const std::byte> encoded) const noexcept override
            {
                if (!encoded.empty())
                {
                    int width = 0;
                    int height = 0;
                    int channels = 0;
                    if (stbi_info_from_memory(reinterpret_cast<const stbi_uc*>(encoded.data()),
                                               static_cast<int>(encoded.size()),
                                               &width,
                                               &height,
                                               &channels) != 0)
                    {
                        return true;
                    }
                }

                const auto extension = normalize_extension(descriptor.source);
                static constexpr std::array<std::string_view, 10> kExtensions{
                    ".png", ".jpg", ".jpeg", ".jpe", ".tga", ".bmp", ".psd", ".gif", ".hdr", ".pic"
                };
                return std::find(kExtensions.begin(), kExtensions.end(), extension) != kExtensions.end();
            }

            [[nodiscard]] engine::Result<TextureDecodedImage, AssetLoadError> decode(
                const TextureAssetDescriptor& descriptor [[maybe_unused]],
                std::span<const std::byte> encoded) const override
            {
                if (encoded.empty())
                {
                    return make_asset_load_error(AssetLoadErrorCategory::DecodeError,
                                                 "Texture payload is empty");
                }

                const stbi_uc* data = reinterpret_cast<const stbi_uc*>(encoded.data());
                const int size = static_cast<int>(encoded.size());

                stbi_set_flip_vertically_on_load(false);

                if (stbi_is_hdr_from_memory(data, size) != 0)
                {
                    int width = 0;
                    int height = 0;
                    int components = 0;
                    float* decoded = stbi_loadf_from_memory(data, size, &width, &height, &components, STBI_rgb_alpha);
                    if (decoded == nullptr)
                    {
                        return make_asset_load_error(AssetLoadErrorCategory::DecodeError,
                                                     stbi_failure_reason() != nullptr
                                                         ? stbi_failure_reason()
                                                         : "Failed to decode HDR texture");
                    }

                    TextureDecodedImage result{};
                    result.format = TextureFormat::rgba32_float;
                    result.extent = TextureDimensions{static_cast<std::uint32_t>(width),
                                                       static_cast<std::uint32_t>(height),
                                                       1U};
                    result.channel_count = 4U;
                    result.high_dynamic_range = true;

                    const std::size_t texel_count = static_cast<std::size_t>(width) * height * result.channel_count;
                    result.pixels.resize(texel_count * sizeof(float));
                    std::memcpy(result.pixels.data(), decoded, result.pixels.size());

                    stbi_image_free(decoded);
                    return result;
                }

                int width = 0;
                int height = 0;
                int components = 0;
                stbi_uc* decoded = stbi_load_from_memory(data, size, &width, &height, &components, STBI_rgb_alpha);
                if (decoded == nullptr)
                {
                    return make_asset_load_error(AssetLoadErrorCategory::DecodeError,
                                                 stbi_failure_reason() != nullptr
                                                     ? stbi_failure_reason()
                                                     : "Failed to decode LDR texture");
                }

                TextureDecodedImage result{};
                result.format = TextureFormat::rgba8_unorm;
                result.extent = TextureDimensions{static_cast<std::uint32_t>(width),
                                                   static_cast<std::uint32_t>(height),
                                                   1U};
                result.channel_count = 4U;
                result.high_dynamic_range = false;

                const std::size_t texel_count = static_cast<std::size_t>(width) * height * result.channel_count;
                result.pixels.resize(texel_count);
                std::memcpy(result.pixels.data(), decoded, result.pixels.size());

                stbi_image_free(decoded);
                return result;
            }
        };
    } // namespace

    namespace detail
    {
        std::unique_ptr<ITextureDecoder> create_stb_texture_decoder()
        {
            return std::make_unique<StbTextureDecoder>();
        }
    } // namespace detail
} // namespace engine::assets
