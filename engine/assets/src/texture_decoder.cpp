#include "engine/assets/texture_decoder.hpp"

#include <algorithm>
#include <cctype>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <span>
#include <vector>

namespace engine::assets
{
    namespace detail
    {
        std::unique_ptr<ITextureDecoder> create_stb_texture_decoder();
    }

    TextureDecoderRegistry& TextureDecoderRegistry::instance()
    {
        static TextureDecoderRegistry registry{};
        return registry;
    }

    TextureDecoderRegistry::TextureDecoderRegistry()
    {
        register_decoder(detail::create_stb_texture_decoder());
    }

    void TextureDecoderRegistry::register_decoder(std::unique_ptr<ITextureDecoder> decoder)
    {
        if (!decoder)
        {
            return;
        }

        std::scoped_lock lock{mutex_};
        decoders_.push_back(std::move(decoder));
    }

    engine::Result<TextureDecodedImage, AssetLoadError> TextureDecoderRegistry::decode(
        const TextureAssetDescriptor& descriptor,
        std::span<const std::byte> encoded) const
    {
        std::vector<ITextureDecoder*> decoders;
        {
            std::scoped_lock lock{mutex_};
            decoders.reserve(decoders_.size());
            for (const auto& decoder : decoders_)
            {
                if (decoder)
                {
                    decoders.push_back(decoder.get());
                }
            }
        }

        std::optional<AssetLoadError> last_error{};

        for (const auto* decoder : decoders)
        {
            if (decoder == nullptr)
            {
                continue;
            }

            if (!decoder->supports(descriptor, encoded))
            {
                continue;
            }

            auto result = decoder->decode(descriptor, encoded);
            if (result.has_value())
            {
                return result;
            }

            last_error = result.error();
        }

        if (last_error.has_value())
        {
            return last_error.value();
        }

        std::ostringstream message;
        message << "No texture decoder available for \"" << descriptor.source.generic_string() << "\"";
        return make_asset_load_error(AssetLoadErrorCategory::DecodeError, message.str());
    }
} // namespace engine::assets
