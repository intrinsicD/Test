#pragma once

#include "engine/io/errors.hpp"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

namespace engine::io::detail
{
    [[nodiscard]] inline std::string to_lower(std::string value)
    {
        std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c)
        {
            return static_cast<char>(std::tolower(c));
        });
        return value;
    }

    class GeometryIoException final : public std::runtime_error
    {
    public:
        GeometryIoException(GeometryIoError code, std::string_view message)
            : std::runtime_error(std::string{message})
              , code_{code}
        {
        }

        [[nodiscard]] GeometryIoError code() const noexcept
        {
            return code_;
        }

    private:
        GeometryIoError code_;
    };

    [[nodiscard]] GeometryIoError map_open_error(const std::filesystem::path& path) noexcept;

    template <typename Callable>
    [[nodiscard]] GeometryIoResult<void> translate_io_exceptions(const std::filesystem::path& path,
                                                                 Callable&& callable)
    {
        try
        {
            std::forward<Callable>(callable)();
            return {};
        }
        catch (const GeometryIoException& e)
        {
            return make_geometry_io_error(e.code(), std::string{e.what()});
        }
        catch (const std::filesystem::filesystem_error& e)
        {
            return make_geometry_io_error(GeometryIoError::io_failure,
                                          std::string{"Filesystem error while processing '"} + path.string()
                                              + "': " + e.what());
        }
        catch (const std::bad_alloc&)
        {
            throw;
        }
        catch (const std::exception& e)
        {
            return make_geometry_io_error(GeometryIoError::invalid_argument,
                                          std::string{"Failed to process '"} + path.string() + "': " + e.what());
        }
    }

    [[nodiscard]] std::vector<std::string> tokenize(const std::string& line);

    void ensure_parent_directory(const std::filesystem::path& path);

    struct PlyHeaderInfo
    {
        std::size_t vertex_count{0};
        std::size_t face_count{0};
        std::size_t edge_count{0};
        bool ascii{true};
    };

    [[nodiscard]] GeometryIoResult<PlyHeaderInfo> inspect_ply_header(const std::filesystem::path& path);
} // namespace engine::io::detail
