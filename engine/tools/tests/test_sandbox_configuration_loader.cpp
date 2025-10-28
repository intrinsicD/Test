#include "engine/tools/sandbox/configuration_loader.hpp"

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>

using namespace engine::tools::sandbox;

namespace
{
    constexpr const char* kSampleSummaryJson = R"json(
{
  "datasets": [
    {
      "id": "geometry-remesh-baseline",
      "kind": "geometry.remesh",
      "label": "Geometry Remesh Baseline",
      "tags": ["geometry", "remesh", "case-study"],
      "source_mesh": "assets/datasets/remesh_sample/source_mesh.obj",
      "output_mesh": "assets/datasets/remesh_sample/output_mesh.obj",
      "statistics": {
        "iterations": 6.0,
        "max_edge_length": 1.118,
        "min_edge_length": 0.5
      },
      "metrics": {
        "input": {
          "edge_length_max": 1.4142,
          "edge_length_min": 1.0
        },
        "output": {
          "edge_length_max": 1.118
        }
      },
      "remeshing_targets": {
        "target_edge_length": 0.5
      },
      "parameterization": {
        "mode": "reuse_existing",
        "texel_density": 256.0
      }
    }
  ],
  "rendering": {
    "preset": "research-baseline",
    "shading_mode": "deferred",
    "resolution": {
      "width": 1280,
      "height": 720
    },
    "overlays": {
      "normals": false,
      "uv": true
    }
  },
  "runtime": {
    "dataset": "geometry-remesh-baseline",
    "camera": {
      "mode": "orbit",
      "position": [0.0, 0.0, 4.0],
      "target": [0.0, 0.0, 0.0]
    },
    "simulation": {
      "timestep_seconds": 0.0166667,
      "max_substeps": 2
    },
    "hot_reload": {
      "enabled": false
    }
  },
  "selected_dataset": "geometry-remesh-baseline"
}
)json";
}

TEST(SandboxConfigurationLoader, ParsesHarnessSummary)
{
    const auto summary = load_summary_from_json(std::string_view{kSampleSummaryJson});

    ASSERT_EQ(summary.datasets.size(), 1U);
    const auto& dataset = summary.datasets.front();
    EXPECT_EQ(dataset.identifier, "geometry-remesh-baseline");
    EXPECT_EQ(dataset.label, "Geometry Remesh Baseline");
    EXPECT_EQ(dataset.kind, "geometry.remesh");
    ASSERT_EQ(dataset.tags.size(), 3U);
    EXPECT_EQ(dataset.tags[0], "geometry");
    ASSERT_TRUE(dataset.statistics.contains("iterations"));
    EXPECT_DOUBLE_EQ(dataset.statistics.at("iterations"), 6.0);
    ASSERT_TRUE(dataset.metrics.contains("input.edge_length_max"));
    EXPECT_NEAR(dataset.metrics.at("input.edge_length_max"), 1.4142, 1e-4);
    ASSERT_TRUE(dataset.metrics.contains("parameterization.texel_density"));
    EXPECT_EQ(dataset.source_asset, "assets/datasets/remesh_sample/source_mesh.obj");
    EXPECT_EQ(dataset.processed_asset, "assets/datasets/remesh_sample/output_mesh.obj");

    ASSERT_EQ(summary.rendering_presets.size(), 1U);
    const auto& preset = summary.rendering_presets.front();
    EXPECT_EQ(preset.identifier, "research-baseline");
    ASSERT_EQ(preset.shading_modes.size(), 1U);
    EXPECT_EQ(preset.shading_modes.front(), "deferred");
    ASSERT_EQ(preset.overlays.size(), 2U);
    EXPECT_EQ(preset.overlays[0].key, "normals");
    EXPECT_FALSE(preset.overlays[0].default_enabled);
    EXPECT_EQ(preset.overlays[1].key, "uv");
    EXPECT_TRUE(preset.overlays[1].default_enabled);
    EXPECT_EQ(preset.default_resolution.first, 1280);
    EXPECT_EQ(preset.default_resolution.second, 720);

    ASSERT_TRUE(summary.selected_dataset.has_value());
    EXPECT_EQ(*summary.selected_dataset, "geometry-remesh-baseline");

    const auto& runtime = summary.runtime;
    EXPECT_EQ(runtime.dataset_identifier, "geometry-remesh-baseline");
    EXPECT_TRUE(runtime.camera_description.find("mode=orbit") != std::string::npos);
    EXPECT_TRUE(runtime.camera_description.find("position=(0.0000, 0.0000, 4.0000)") != std::string::npos);
    EXPECT_TRUE(runtime.simulation_description.find("dt=0.016667") != std::string::npos);
    EXPECT_TRUE(runtime.simulation_description.find("max_substeps=2") != std::string::npos);
    EXPECT_FALSE(runtime.hot_reload_enabled);
}

TEST(SandboxConfigurationLoader, LoadsSummaryFromFile)
{
    const auto temp_path = std::filesystem::temp_directory_path() / "sandbox_summary.json";
    {
        std::ofstream stream(temp_path, std::ios::trunc);
        ASSERT_TRUE(stream.is_open());
        stream << kSampleSummaryJson;
    }

    const auto summary = load_summary_from_json(temp_path);
    EXPECT_EQ(summary.datasets.size(), 1U);

    std::error_code ec;
    std::filesystem::remove(temp_path, ec);
}
