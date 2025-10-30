#include "engine/tools/sandbox/configuration_loader.hpp"

#include <gtest/gtest.h>

#include <algorithm>
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
      "schema_id": "ai004.dataset.geometry.remesh",
      "schema_version": 3,
      "label": "Geometry Remesh Baseline",
      "tags": ["geometry", "remesh", "case-study"],
      "source_generator": "scripts/datasets/ingest_dataset.py",
      "source_mesh": "assets/datasets/remesh_sample/source_mesh.obj",
      "source_mesh_sha256": "source-hash",
      "source_mesh_size_bytes": 2048,
      "output_mesh": "assets/datasets/remesh_sample/output_mesh.obj",
      "output_mesh_sha256": "output-hash",
      "output_mesh_size_bytes": 4096,
      "remeshing_mode": "adaptive",
      "statistics": {
        "iterations": 6,
        "max_edge_length": 1.118,
        "min_edge_length": 0.5
      },
      "metrics": {
        "input": {
          "vertices": 4,
          "faces": 2,
          "edge_length": {
            "min": 1.0,
            "mean": 1.1381,
            "max": 1.4142
          }
        },
        "output": {
          "vertices": 5,
          "faces": 4,
          "edge_length": {
            "min": 0.5,
            "mean": 0.75,
            "max": 1.118
          }
        }
      },
      "remeshing_targets": {
        "target_edge_length": 0.5
      },
      "parameterization": {
        "mode": "reuse_existing",
        "texel_density": 256.0
      },
      "feature_preservation": {
        "lock_boundary_edges": true,
        "minimum_feature_angle_degrees": 30.0,
        "lock_feature_edges": false
      },
      "assets": [
        {
          "role": "source",
          "path": "assets/datasets/remesh_sample/source_mesh.obj",
          "resolved_path": "/tmp/remesh/source_mesh.obj",
          "exists": true,
          "verified": true
        },
        {
          "role": "output",
          "path": "assets/datasets/remesh_sample/output_mesh.obj",
          "resolved_path": "/tmp/remesh/output_mesh.obj",
          "exists": false,
          "verified": false,
          "expected_size_bytes": 1024,
          "actual_size_bytes": 512,
          "expected_sha256": "expected",
          "actual_sha256": "actual",
          "message": "asset mismatch"
        }
      ]
    }
  ],
  "rendering_presets": [
    {
      "id": "research-baseline",
      "label": "Research Baseline",
      "shading_modes": ["deferred"],
      "overlays": [
        {"key": "normals", "label": "Normals", "default_enabled": false},
        {"key": "uv", "label": "UV", "default_enabled": true}
      ],
      "default_resolution": {"width": 1280, "height": 720}
    }
  ],
  "selected_rendering_preset": "research-baseline",
  "algorithm_variants": [
    {"id": "baseline", "label": "Baseline", "description": "Reference runtime profile"}
  ],
  "selected_algorithm_variant": "baseline",
  "runtime": {
    "schema_version": 4,
    "dataset": "geometry-remesh-baseline",
    "scene_manifest_path": "/tmp/runtime_scene.json",
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
      "enabled": false,
      "watch_interval_seconds": 0.25
    }
  },
  "selected_dataset": "geometry-remesh-baseline",
  "telemetry": {
    "schema_version": 2,
    "outputs": [
      {
        "kind": "file",
        "path": "/tmp/telemetry.json",
        "template": "telemetry/{scenario}.json"
      },
      {
        "kind": "stdout"
      }
    ],
    "metrics": [
      {
        "name": "frame_time",
        "statistic": "avg"
      },
      {
        "name": "gpu_time",
        "statistic": "p95"
      }
    ],
    "sampling": {
      "frame_interval": 8,
      "include_debug_overlays": true
    }
  },
  "benchmarks": {
    "schema_version": 1,
    "scenarios": [
      {
        "id": "remesh-baseline",
        "name": "Remesh Baseline",
        "dataset": "geometry-remesh-baseline",
        "rendering_preset": "research-baseline",
        "runtime_profile": "default",
        "engine": {
          "command": ["python", "run_engine.py"],
          "output": "telemetry/{scenario}_engine.json"
        },
        "reference": {
          "command": ["python", "run_reference.py"],
          "output": "telemetry/{scenario}_reference.json"
        },
        "metrics": [
          {
            "name": "frame_time",
            "higher_is_better": false,
            "threshold": {
              "mode": "relative",
              "limit": 0.05
            }
          }
        ]
      }
    ]
  }
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
    EXPECT_EQ(dataset.schema_id, "ai004.dataset.geometry.remesh");
    EXPECT_EQ(dataset.schema_version, 3);
    ASSERT_EQ(dataset.tags.size(), 3U);
    EXPECT_EQ(dataset.tags[0], "geometry");
    ASSERT_TRUE(dataset.statistics.contains("iterations"));
    EXPECT_DOUBLE_EQ(dataset.statistics.at("iterations"), 6.0);
    ASSERT_TRUE(dataset.metrics.contains("input.edge_length.max"));
    EXPECT_NEAR(dataset.metrics.at("input.edge_length.max"), 1.4142, 1e-4);
    ASSERT_TRUE(dataset.metrics.contains("output.vertices"));
    EXPECT_DOUBLE_EQ(dataset.metrics.at("output.vertices"), 5.0);
    ASSERT_TRUE(dataset.metrics.contains("parameterization.texel_density"));
    EXPECT_EQ(dataset.source_generator, "scripts/datasets/ingest_dataset.py");
    EXPECT_EQ(dataset.source_asset, "assets/datasets/remesh_sample/source_mesh.obj");
    ASSERT_TRUE(dataset.source_asset_sha256.has_value());
    EXPECT_EQ(dataset.source_asset_sha256.value(), "source-hash");
    ASSERT_TRUE(dataset.source_asset_size_bytes.has_value());
    EXPECT_EQ(dataset.source_asset_size_bytes.value(), 2048U);
    EXPECT_EQ(dataset.processed_asset, "assets/datasets/remesh_sample/output_mesh.obj");
    ASSERT_TRUE(dataset.processed_asset_sha256.has_value());
    EXPECT_EQ(dataset.processed_asset_sha256.value(), "output-hash");
    ASSERT_TRUE(dataset.processed_asset_size_bytes.has_value());
    EXPECT_EQ(dataset.processed_asset_size_bytes.value(), 4096U);
    EXPECT_EQ(dataset.remeshing_mode, "adaptive");
    ASSERT_EQ(dataset.remeshing_targets.size(), 1U);
    EXPECT_EQ(dataset.remeshing_targets.front().first, "target_edge_length");
    EXPECT_NEAR(dataset.remeshing_targets.front().second, 0.5, 1e-6);
    ASSERT_EQ(dataset.feature_preservation.size(), 3U);
    const auto find_feature = [&](std::string_view key) {
        return std::find_if(dataset.feature_preservation.begin(), dataset.feature_preservation.end(),
                            [&](const auto& entry) { return entry.first == key; });
    };
    auto lock_boundary = find_feature("lock_boundary_edges");
    ASSERT_NE(lock_boundary, dataset.feature_preservation.end());
    EXPECT_EQ(lock_boundary->second, "true");
    auto lock_feature = find_feature("lock_feature_edges");
    ASSERT_NE(lock_feature, dataset.feature_preservation.end());
    EXPECT_EQ(lock_feature->second, "false");
    auto feature_angle = find_feature("minimum_feature_angle_degrees");
    ASSERT_NE(feature_angle, dataset.feature_preservation.end());
    EXPECT_EQ(feature_angle->second, "30.000");
    ASSERT_EQ(dataset.parameterization_properties.size(), 1U);
    EXPECT_EQ(dataset.parameterization_properties.front().first, "mode");
    EXPECT_EQ(dataset.parameterization_properties.front().second, "reuse_existing");
    ASSERT_EQ(dataset.assets.size(), 2U);
    EXPECT_TRUE(dataset.assets[0].verified);
    EXPECT_FALSE(dataset.assets[0].message.has_value());
    EXPECT_FALSE(dataset.assets[1].verified);
    ASSERT_TRUE(dataset.assets[1].actual_size_bytes.has_value());
    EXPECT_EQ(dataset.assets[1].actual_size_bytes.value(), 512U);
    ASSERT_TRUE(dataset.assets[1].message.has_value());
    EXPECT_EQ(dataset.assets[1].message.value(), "asset mismatch");

    ASSERT_EQ(summary.rendering_presets.size(), 1U);
    const auto& preset = summary.rendering_presets.front();
    EXPECT_EQ(preset.identifier, "research-baseline");
    ASSERT_TRUE(summary.selected_rendering_preset.has_value());
    EXPECT_EQ(*summary.selected_rendering_preset, "research-baseline");
    ASSERT_EQ(preset.shading_modes.size(), 1U);
    EXPECT_EQ(preset.shading_modes.front(), "deferred");
    ASSERT_EQ(preset.overlays.size(), 2U);
    EXPECT_EQ(preset.overlays[0].key, "normals");
    EXPECT_EQ(preset.overlays[0].label, "Normals");
    EXPECT_FALSE(preset.overlays[0].default_enabled);
    EXPECT_EQ(preset.overlays[1].key, "uv");
    EXPECT_EQ(preset.overlays[1].label, "UV");
    EXPECT_TRUE(preset.overlays[1].default_enabled);
    EXPECT_EQ(preset.default_resolution.first, 1280);
    EXPECT_EQ(preset.default_resolution.second, 720);

    ASSERT_EQ(summary.algorithm_variants.size(), 1U);
    const auto& variant = summary.algorithm_variants.front();
    EXPECT_EQ(variant.identifier, "baseline");
    EXPECT_EQ(variant.label, "Baseline");
    ASSERT_TRUE(variant.description.has_value());
    EXPECT_EQ(*variant.description, "Reference runtime profile");
    ASSERT_TRUE(summary.selected_algorithm_variant.has_value());
    EXPECT_EQ(*summary.selected_algorithm_variant, "baseline");

    ASSERT_TRUE(summary.selected_dataset.has_value());
    EXPECT_EQ(*summary.selected_dataset, "geometry-remesh-baseline");

    const auto& runtime = summary.runtime;
    EXPECT_EQ(runtime.schema_version, 4);
    EXPECT_EQ(runtime.dataset_identifier, "geometry-remesh-baseline");
    EXPECT_TRUE(runtime.camera_description.find("mode=orbit") != std::string::npos);
    EXPECT_TRUE(runtime.camera_description.find("position=(0.0000, 0.0000, 4.0000)") != std::string::npos);
    EXPECT_TRUE(runtime.simulation_description.find("dt=0.016667") != std::string::npos);
    EXPECT_TRUE(runtime.simulation_description.find("max_substeps=2") != std::string::npos);
    ASSERT_TRUE(runtime.scene_manifest_path.has_value());
    EXPECT_EQ(runtime.scene_manifest_path.value(), "/tmp/runtime_scene.json");
    EXPECT_FALSE(runtime.hot_reload.enabled);
    ASSERT_TRUE(runtime.hot_reload.watch_interval_seconds.has_value());
    EXPECT_NEAR(runtime.hot_reload.watch_interval_seconds.value(), 0.25, 1e-6);

    ASSERT_TRUE(summary.telemetry.has_value());
    const auto& telemetry = *summary.telemetry;
    EXPECT_EQ(telemetry.schema_version, 2);
    ASSERT_EQ(telemetry.outputs.size(), 2U);
    EXPECT_EQ(telemetry.outputs[0].kind, "file");
    ASSERT_TRUE(telemetry.outputs[0].path.has_value());
    EXPECT_EQ(telemetry.outputs[0].path.value(), "/tmp/telemetry.json");
    ASSERT_TRUE(telemetry.outputs[0].template_path.has_value());
    EXPECT_EQ(telemetry.outputs[0].template_path.value(), "telemetry/{scenario}.json");
    EXPECT_EQ(telemetry.outputs[1].kind, "stdout");
    EXPECT_FALSE(telemetry.outputs[1].path.has_value());
    ASSERT_TRUE(telemetry.metrics.size() >= 2U);
    EXPECT_EQ(telemetry.metrics[0].name, "frame_time");
    EXPECT_EQ(telemetry.metrics[0].statistic, "avg");
    ASSERT_TRUE(telemetry.sampling.has_value());
    EXPECT_EQ(telemetry.sampling->frame_interval, 8);
    EXPECT_TRUE(telemetry.sampling->include_debug_overlays);

    ASSERT_TRUE(summary.benchmarks.has_value());
    const auto& benchmarks = *summary.benchmarks;
    EXPECT_EQ(benchmarks.schema_version, 1);
    ASSERT_EQ(benchmarks.scenarios.size(), 1U);
    const auto& scenario = benchmarks.scenarios.front();
    EXPECT_EQ(scenario.identifier, "remesh-baseline");
    EXPECT_EQ(scenario.rendering_preset, "research-baseline");
    ASSERT_EQ(scenario.engine.command.size(), 2U);
    EXPECT_EQ(scenario.engine.command.front(), "python");
    ASSERT_EQ(scenario.metrics.size(), 1U);
    EXPECT_EQ(scenario.metrics.front().threshold.mode, "relative");
    EXPECT_NEAR(scenario.metrics.front().threshold.limit, 0.05, 1e-6);
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
