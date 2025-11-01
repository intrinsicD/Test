#include "engine/assets/texture_asset.hpp"

#include "engine/assets/detail/filesystem_utils.hpp"
#include "engine/assets/detail/reload_utils.hpp"
#include "engine/assets/texture_decoder.hpp"
#include "engine/assets/validation.hpp"

#include <algorithm>
#include <bit>
#include <cassert>
#include <cmath>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <vector>
#include <span>
#include <utility>

namespace engine::assets
{
    namespace
    {
        void read_binary(const std::filesystem::path& path, std::vector<std::byte>& output)
        {
            std::ifstream stream{path, std::ios::binary};
            if (!stream)
            {
                throw std::runtime_error("Failed to open texture file: " + path.generic_string());
            }

            stream.seekg(0, std::ios::end);
            const auto size = static_cast<std::size_t>(stream.tellg());
            stream.seekg(0, std::ios::beg);
            output.resize(size);
            if (!stream.read(reinterpret_cast<char*>(output.data()), static_cast<std::streamsize>(size)))
            {
                throw std::runtime_error("Failed to read texture file: " + path.generic_string());
            }
        }

        [[nodiscard]] TextureDimensions next_extent(TextureDimensions extent) noexcept
        {
            return TextureDimensions{
                std::max(1U, extent.width / 2U),
                std::max(1U, extent.height / 2U),
                std::max(1U, extent.depth / 2U)
            };
        }

        [[nodiscard]] std::vector<std::byte> downsample(TextureFormat format,
                                                         const TextureMipLevel& previous)
        {
            const auto channels = texture_channel_count(format);
            const auto bytes_per_pixel = texture_bytes_per_pixel(format);
            if (channels == 0 || bytes_per_pixel == 0)
            {
                throw std::runtime_error("Unsupported texture format for mipmap generation");
            }

            if (previous.extent.depth != 1)
            {
                throw std::runtime_error("Mipmap generation only supports 2D textures");
            }

            const TextureDimensions extent = next_extent(previous.extent);
            std::vector<std::byte> output(static_cast<std::size_t>(extent.width) * extent.height * bytes_per_pixel);
            std::vector<double> accumulator(channels, 0.0);

            const auto bytes_per_channel = bytes_per_pixel / channels;
            const auto parent_stride = static_cast<std::size_t>(previous.extent.width) * bytes_per_pixel;

            for (std::uint32_t y = 0; y < extent.height; ++y)
            {
                for (std::uint32_t x = 0; x < extent.width; ++x)
                {
                    std::fill(accumulator.begin(), accumulator.end(), 0.0);
                    std::uint32_t samples = 0;

                    for (std::uint32_t offset_y = 0; offset_y < 2; ++offset_y)
                    {
                        const auto src_y = std::min(previous.extent.height - 1, 2U * y + offset_y);
                        if (src_y >= previous.extent.height)
                        {
                            continue;
                        }

                        for (std::uint32_t offset_x = 0; offset_x < 2; ++offset_x)
                        {
                            const auto src_x = std::min(previous.extent.width - 1, 2U * x + offset_x);
                            if (src_x >= previous.extent.width)
                            {
                                continue;
                            }

                            const auto base = static_cast<std::size_t>(src_y) * parent_stride
                                + static_cast<std::size_t>(src_x) * bytes_per_pixel;

                            for (std::uint32_t channel = 0; channel < channels; ++channel)
                            {
                                const auto channel_offset = base + channel * bytes_per_channel;
                                if (bytes_per_channel == 1)
                                {
                                    accumulator[channel] += static_cast<double>(std::to_integer<unsigned char>(
                                        previous.texels[channel_offset]));
                                }
                                else if (bytes_per_channel == 4)
                                {
                                    float value = 0.0F;
                                    std::memcpy(&value, previous.texels.data() + channel_offset, sizeof(float));
                                    accumulator[channel] += static_cast<double>(value);
                                }
                                else
                                {
                                    throw std::runtime_error("Unsupported bytes-per-channel for mipmap generation");
                                }
                            }

                            ++samples;
                        }
                    }

                    samples = std::max<std::uint32_t>(1, samples);
                    const auto destination_base = (static_cast<std::size_t>(y) * extent.width + x) * bytes_per_pixel;
                    for (std::uint32_t channel = 0; channel < channels; ++channel)
                    {
                        const double averaged = accumulator[channel] / static_cast<double>(samples);
                        if (bytes_per_channel == 1)
                        {
                            const auto value = static_cast<unsigned char>(
                                std::clamp<std::int32_t>(static_cast<std::int32_t>(std::lround(averaged)), 0, 255));
                            output[destination_base + channel] = static_cast<std::byte>(value);
                        }
                        else if (bytes_per_channel == 4)
                        {
                            const float value = static_cast<float>(averaged);
                            std::memcpy(output.data() + destination_base + channel * bytes_per_channel,
                                        &value,
                                        sizeof(float));
                        }
                    }
                }
            }

            return output;
        }

        [[nodiscard]] std::vector<TextureMipLevel> generate_mip_chain(TextureDecodedImage decoded,
                                                                      const TextureLoadingOptions& options,
                                                                      TextureFormat format)
        {
            std::vector<TextureMipLevel> mip_chain{};
            mip_chain.reserve(options.generate_mipmaps ? 8U : 1U);

            TextureMipLevel base{};
            base.extent = decoded.extent;
            base.texels = std::move(decoded.pixels);
            mip_chain.push_back(std::move(base));

            if (!options.generate_mipmaps)
            {
                return mip_chain;
            }

            const std::uint32_t max_supported = compute_max_mip_levels(decoded.extent);
            std::uint32_t target_levels = max_supported;
            if (options.max_mip_levels != 0)
            {
                target_levels = std::min(options.max_mip_levels, max_supported);
                target_levels = std::max<std::uint32_t>(target_levels, 1U);
            }

            while (mip_chain.size() < target_levels)
            {
                const auto& previous = mip_chain.back();
                TextureMipLevel next{};
                next.extent = next_extent(previous.extent);
                if (next.extent.width == previous.extent.width
                    && next.extent.height == previous.extent.height
                    && next.extent.depth == previous.extent.depth)
                {
                    break;
                }
                next.texels = downsample(format, previous);
                mip_chain.push_back(std::move(next));
            }

            return mip_chain;
        }

        [[nodiscard]] bool validate_decoded_image(const TextureDecodedImage& decoded)
        {
            if (decoded.format == TextureFormat::unknown)
            {
                return false;
            }

            if (decoded.extent.width == 0 || decoded.extent.height == 0 || decoded.extent.depth == 0)
            {
                return false;
            }

            if (decoded.extent.depth != 1)
            {
                return false;
            }

            const auto expected_channels = texture_channel_count(decoded.format);
            const auto expected_bytes_per_pixel = texture_bytes_per_pixel(decoded.format);
            if (expected_channels == 0 || expected_bytes_per_pixel == 0)
            {
                return false;
            }

            const auto expected_size = static_cast<std::size_t>(decoded.extent.width)
                * decoded.extent.height * expected_bytes_per_pixel;
            if (decoded.pixels.size() != expected_size)
            {
                return false;
            }

            if (decoded.channel_count != expected_channels)
            {
                return false;
            }

            return true;
        }
    } // namespace

    std::uint32_t texture_channel_count(TextureFormat format) noexcept
    {
        switch (format)
        {
        case TextureFormat::rgba8_unorm:
        case TextureFormat::rgba32_float:
            return 4U;
        case TextureFormat::unknown:
        default:
            break;
        }
        return 0U;
    }

    std::uint32_t texture_bytes_per_pixel(TextureFormat format) noexcept
    {
        switch (format)
        {
        case TextureFormat::rgba8_unorm:
            return 4U;
        case TextureFormat::rgba32_float:
            return 16U;
        case TextureFormat::unknown:
        default:
            break;
        }
        return 0U;
    }

    std::uint32_t compute_max_mip_levels(TextureDimensions extent) noexcept
    {
        const std::uint32_t max_dimension = std::max({extent.width, extent.height, extent.depth});
        if (max_dimension == 0)
        {
            return 0;
        }

        std::uint32_t levels = 1;
        std::uint32_t size = max_dimension;
        while (size > 1)
        {
            size = std::max(1U, size / 2U);
            ++levels;
            if (size == 1)
            {
                break;
            }
        }
        return levels;
    }

    TextureCache::TextureCache()
        : Base(detail::AssetCacheLabels{
              "Texture",
              "texture",
              "TextureHandle",
              "TextureCache"})
        , handle_validator_registration_(HandleValidatorRegistry::instance().register_texture_validator(
            [this](const TextureHandle& handle)
            {
                std::scoped_lock lock{this->mutex_};
                return handle.is_valid(this->assets_);
            }))
    {
    }

    const TextureAsset& TextureCache::load(const TextureAssetDescriptor& descriptor)
    {
        std::scoped_lock lock{this->mutex_};

        auto acquisition = this->acquire_asset_slot(descriptor);
        this->bind_descriptor(descriptor, acquisition.handle, *acquisition.asset);
        this->merge_pending_callbacks(acquisition.identifier, acquisition.handle);

        const auto decision = this->evaluate_reload(descriptor, *acquisition.asset, acquisition.inserted);
        if (decision.should_reload)
        {
            if (auto reload = reload_asset(acquisition.handle, *acquisition.asset, !acquisition.inserted); !reload.has_value())
            {
                const auto message = reload.error().message();
                throw std::runtime_error(message.empty()
                                             ? std::string{to_string(reload.error().code())}
                                             : std::string{message});
            }
        }

        this->register_watch_locked(acquisition.handle, *acquisition.asset);

        return *acquisition.asset;
    }

    bool TextureCache::contains(const TextureHandle& handle) const
    {
        std::scoped_lock lock{this->mutex_};
        return this->contains_handle(handle);
    }

    const TextureAsset& TextureCache::get(const TextureHandle& handle) const
    {
        std::scoped_lock lock{this->mutex_};
        return this->get_asset_checked(handle);
    }

    void TextureCache::unload(const TextureHandle& handle)
    {
        std::scoped_lock lock{this->mutex_};
        this->release_handle(handle);
    }

    void TextureCache::register_hot_reload_callback(const TextureHandle& handle, HotReloadCallback callback)
    {
        std::scoped_lock lock{this->mutex_};
        this->register_hot_reload_callback_internal(handle, std::move(callback));
    }

    void TextureCache::poll()
    {
        this->poll_assets();
    }

    engine::Result<void, AssetLoadError> TextureCache::reload_asset(const RawHandle& handle,
                                                                    TextureAsset& asset,
                                                                    bool notify)
    {
        const std::string identifier = asset.descriptor.handle.id();
        detail::record_hot_reload_attempt(notify, identifier);

        std::vector<std::byte> loaded_data{};
        try
        {
            read_binary(asset.descriptor.source, loaded_data);
        }
        catch (const std::exception& ex)
        {
            auto error = make_asset_load_error(AssetLoadErrorCategory::IoFailure, ex.what());
            detail::record_hot_reload_failure(
                notify,
                identifier,
                error,
                "Verify the texture path exists and is readable by the runtime.");
            return error;
        }

        std::filesystem::file_time_type last_write{};
        try
        {
            last_write = detail::checked_last_write_time(asset.descriptor.source, "texture");
        }
        catch (const std::runtime_error& ex)
        {
            auto error = make_asset_load_error(AssetLoadErrorCategory::IoFailure, ex.what());
            detail::record_hot_reload_failure(
                notify,
                identifier,
                error,
                "Ensure the texture file remains on disk and the watcher has permission to read it.");
            return error;
        }

        auto decoded_result = TextureDecoderRegistry::instance().decode(
            asset.descriptor,
            std::span<const std::byte>{loaded_data.data(), loaded_data.size()});
        if (!decoded_result)
        {
            auto error = decoded_result.error();
            detail::record_hot_reload_failure(
                notify,
                identifier,
                error,
                "Confirm the texture encoding is supported and not corrupted.");
            return error;
        }

        if (!validate_decoded_image(decoded_result.value()))
        {
            auto error = make_asset_load_error(AssetLoadErrorCategory::DecodeError,
                                               "Texture decoder produced invalid metadata");
            detail::record_hot_reload_failure(
                notify,
                identifier,
                error,
                "Re-export the texture with a supported format (PNG, JPEG, HDR).");
            return error;
        }

        TextureDecodedImage image = std::move(decoded_result).value();
        asset.format = image.format;
        asset.dimensions = image.extent;
        asset.mip_levels = generate_mip_chain(image, asset.descriptor.options, asset.format);

        if (asset.descriptor.options.retain_encoded_payload)
        {
            asset.encoded_payload = std::move(loaded_data);
        }
        else
        {
            asset.encoded_payload.clear();
        }

        asset.last_write = last_write;

        if (notify)
        {
            const auto cb_it = this->callbacks_.find(handle);
            if (cb_it != this->callbacks_.end())
            {
                for (const auto& callback : cb_it->second)
                {
                    callback(asset);
                }
            }
        }

        return {};
    }
} // namespace engine::assets
