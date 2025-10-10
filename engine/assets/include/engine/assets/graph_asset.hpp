#pragma once

#include "engine/assets/handles.hpp"

#include "engine/io/geometry_io.hpp"

#include "engine/geometry/graph/graph.hpp"

#include <filesystem>
#include <functional>
#include <unordered_map>
#include <vector>

namespace engine::assets
{
    struct GraphAssetDescriptor
    {
        GraphHandle handle{};
        std::filesystem::path source{};
        io::GraphFileFormat format_hint{io::GraphFileFormat::unknown};

        [[nodiscard]] static GraphAssetDescriptor from_file(const std::filesystem::path& path,
                                                            io::GraphFileFormat hint = io::GraphFileFormat::unknown)
        {
            return GraphAssetDescriptor{GraphHandle{path}, path, hint};
        }
    };

    struct GraphAsset
    {
        GraphAssetDescriptor descriptor{};
        geometry::Graph graph{};
        io::GeometryDetectionResult detection{};
        std::filesystem::file_time_type last_write{};
    };

    class GraphCache
    {
    public:
        using HotReloadCallback = std::function<void(const GraphAsset&)>;

        [[nodiscard]] const GraphAsset& load(const GraphAssetDescriptor& descriptor);
        [[nodiscard]] bool contains(const GraphHandle& handle) const;
        [[nodiscard]] const GraphAsset& get(const GraphHandle& handle) const;

        void unload(const GraphHandle& handle);
        void register_hot_reload_callback(const GraphHandle& handle, HotReloadCallback callback);
        void poll();

    private:
        void reload_asset(const GraphHandle& handle, GraphAsset& asset, bool notify);

        std::unordered_map<GraphHandle, GraphAsset> assets_{};
        std::unordered_map<GraphHandle, std::vector<HotReloadCallback>> callbacks_{};
    };
} // namespace engine::assets

