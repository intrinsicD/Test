#include <gtest/gtest.h>

#include "engine/runtime/config_schema.hpp"

#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <optional>
#include <string>
#include <string_view>
#include <system_error>

namespace
{
#if defined(_WIN32)
    std::optional<std::string> get_env_variable(const char* name)
    {
        char* value = nullptr;
        size_t length = 0;
        if (_dupenv_s(&value, &length, name) != 0 || value == nullptr)
        {
            return std::nullopt;
        }
        std::string result{value};
        free(value);
        return result;
    }

    void set_env_variable(const char* name, const std::string& value)
    {
        _putenv_s(name, value.c_str());
    }

    void unset_env_variable(const char* name)
    {
        _putenv_s(name, "");
    }
#else
    std::optional<std::string> get_env_variable(const char* name)
    {
        const char* value = std::getenv(name);
        if (value == nullptr)
        {
            return std::nullopt;
        }
        return std::string{value};
    }

    void set_env_variable(const char* name, const std::string& value)
    {
        ::setenv(name, value.c_str(), 1);
    }

    void unset_env_variable(const char* name)
    {
        ::unsetenv(name);
    }
#endif

    class EnvVarGuard
    {
    public:
        EnvVarGuard(const char* name, std::optional<std::string> replacement)
            : name_{name},
              previous_{get_env_variable(name)}
        {
            if (replacement.has_value())
            {
                set_env_variable(name_, *replacement);
            }
            else
            {
                unset_env_variable(name_);
            }
        }

        EnvVarGuard(const EnvVarGuard&) = delete;
        EnvVarGuard& operator=(const EnvVarGuard&) = delete;

        ~EnvVarGuard()
        {
            if (previous_.has_value())
            {
                set_env_variable(name_, *previous_);
            }
            else
            {
                unset_env_variable(name_);
            }
        }

    private:
        const char* name_;
        std::optional<std::string> previous_;
    };

    struct TempDirectory
    {
        TempDirectory()
        {
            const auto timestamp = std::chrono::steady_clock::now().time_since_epoch().count();
            path = std::filesystem::temp_directory_path() /
                ("engine-runtime-config-schema-" + std::to_string(timestamp));
            std::filesystem::create_directories(path);
        }

        TempDirectory(const TempDirectory&) = delete;
        TempDirectory& operator=(const TempDirectory&) = delete;

        ~TempDirectory()
        {
            std::error_code ec;
            std::filesystem::remove_all(path, ec);
        }

        std::filesystem::path path;
    };

    std::filesystem::path write_file(const std::filesystem::path& path, std::string_view contents)
    {
        std::filesystem::create_directories(path.parent_path());
        std::ofstream stream{path, std::ios::binary};
        EXPECT_TRUE(stream.good());
        stream << contents;
        stream.flush();
        EXPECT_TRUE(stream.good());
        return path;
    }
} // namespace

using engine::runtime::RuntimeError;
using engine::runtime::config::Ai004Configuration;
using engine::runtime::config::DatasetManifest;
using engine::runtime::config::load_configuration;
using engine::runtime::config::load_dataset_manifest;
using namespace std::string_view_literals;

TEST(RuntimeConfigSchema, LoadDatasetManifestWithoutSchemaInjectsDefaults)
{
    TempDirectory temp;
    const auto manifest_path = write_file(
        temp.path / "manifest.yaml",
        R"(datasets:
  - id: remesh-sample
    kind: geometry.remesh
    tags: [geometry]
    source:
      generator: geometry_remesh
      mesh: input.obj
    outputs:
      mesh: output.obj
    remeshing:
      mode: uniform
    feature_preservation:
      lock_boundary_edges: true
      lock_feature_edges: false
      minimum_feature_angle_degrees: 35.0
    metrics:
      input:
        vertices: 8
        faces: 12
        edge_length:
          min: 0.4
          max: 1.2
          mean: 0.8
      output:
        vertices: 16
        faces: 24
        edge_length:
          min: 0.2
          max: 0.9
          mean: 0.4
    statistics:
      iterations: 3
      max_error: 0.02
      min_edge_length: 0.2
      max_edge_length: 0.9
      max_surface_deviation: 0.01
      mean_surface_deviation: 0.008
      rms_surface_deviation: 0.009
)"sv);

    auto result = load_dataset_manifest(manifest_path);
    ASSERT_TRUE(result);
    const DatasetManifest& manifest = result.value();
    ASSERT_EQ(manifest.datasets.size(), 1U);
    const auto& dataset = manifest.datasets.front();
    EXPECT_EQ(dataset.schema_id, "ai-004.dataset");
    EXPECT_EQ(dataset.schema_version, 1);
    EXPECT_FALSE(dataset.source_mesh_sha256.has_value());
    EXPECT_FALSE(dataset.output_mesh_sha256.has_value());
}

TEST(RuntimeConfigSchema, LoadDatasetManifestHonoursSchemaFlag)
{
    TempDirectory temp;
    const auto manifest_path = write_file(
        temp.path / "manifest.yaml",
        R"(datasets:
  - id: remesh-sample
    kind: geometry.remesh
    tags: [geometry]
    source:
      generator: geometry_remesh
      mesh: input.obj
    outputs:
      mesh: output.obj
    remeshing:
      mode: uniform
    feature_preservation:
      lock_boundary_edges: true
      lock_feature_edges: true
      minimum_feature_angle_degrees: 25.0
    metrics:
      input:
        vertices: 4
        faces: 4
        edge_length:
          min: 0.2
          max: 0.6
          mean: 0.4
      output:
        vertices: 8
        faces: 12
        edge_length:
          min: 0.1
          max: 0.5
          mean: 0.3
    statistics:
      iterations: 2
      max_error: 0.01
      min_edge_length: 0.1
      max_edge_length: 0.5
      max_surface_deviation: 0.008
      mean_surface_deviation: 0.006
      rms_surface_deviation: 0.007
)"sv);

    {
        EnvVarGuard guard{"ENGINE_AI004_SCHEMA_V1", std::nullopt};
        auto result = load_dataset_manifest(manifest_path);
        ASSERT_TRUE(result);
    }

    EnvVarGuard guard{"ENGINE_AI004_SCHEMA_V1", std::string{"1"}};
    auto enforced = load_dataset_manifest(manifest_path);
    ASSERT_FALSE(enforced);
    EXPECT_EQ(enforced.error().code(), RuntimeError::configuration_validation_error);
    EXPECT_TRUE(enforced.error().message().find("schema") != std::string::npos);
}

TEST(RuntimeConfigSchema, LoadConfigurationParsesSections)
{
    TempDirectory temp;
    const auto config_path = write_file(
        temp.path / "config.yaml",
        R"(datasets:
  - id: remesh-sample
    schema:
      id: ai-004.dataset
      version: 2
    kind: geometry.remesh
    tags: [geometry, remesh]
    source:
      generator: geometry_remesh
      mesh: input.obj
      mesh_sha256: ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff
      mesh_size_bytes: 1024
    outputs:
      mesh: output.obj
      mesh_sha256: 0000000000000000000000000000000000000000000000000000000000000000
      mesh_size_bytes: 2048
    remeshing:
      mode: uniform
    feature_preservation:
      lock_boundary_edges: true
      lock_feature_edges: true
      minimum_feature_angle_degrees: 30.0
    metrics:
      input:
        vertices: 8
        faces: 12
        edge_length:
          min: 0.4
          max: 1.2
          mean: 0.8
      output:
        vertices: 16
        faces: 24
        edge_length:
          min: 0.2
          max: 0.9
          mean: 0.5
    statistics:
      iterations: 4
      max_error: 0.01
      min_edge_length: 0.2
      max_edge_length: 0.9
      max_surface_deviation: 0.008
      mean_surface_deviation: 0.006
      rms_surface_deviation: 0.007
rendering:
  schema:
    id: ai-004.rendering
    version: 1
  preset: research-baseline
  options:
    shading_mode: deferred
    resolution:
      width: 1920
      height: 1080
runtime:
  schema:
    id: ai-004.runtime
    version: 1
  dataset: remesh-sample
  scene:
    manifest: scenes/sample.scene
    entry_point: main
  hot_reload:
    enabled: true
    watch_interval_seconds: 0.5
benchmarks:
  schema:
    id: ai-004.benchmarks
    version: 1
  scenarios:
    - name: Baseline
      id: baseline
      dataset: remesh-sample
      rendering_preset: research-baseline
      engine:
        command: ["python", "engine.py"]
        output: outputs/engine.json
      reference:
        command: ["python", "reference.py"]
        output: outputs/reference.json
      metrics:
        - name: fps
          higher_is_better: true
          threshold:
            type: relative
            max_regression: 0.05
telemetry:
  schema:
    id: ai-004.telemetry
    version: 1
  outputs:
    - type: file
      path: telemetry/run.json
  metrics:
    - name: frame_time
      statistic: mean
  sampling:
    frame_interval: 2
    include_debug_overlays: false
)"sv);

    auto result = load_configuration(config_path);
    ASSERT_TRUE(result);
    const Ai004Configuration& configuration = result.value();
    ASSERT_EQ(configuration.datasets.datasets.size(), 1U);
    ASSERT_TRUE(configuration.rendering.has_value());
    EXPECT_EQ(configuration.rendering->shading_mode, "deferred");
    ASSERT_TRUE(configuration.runtime.has_value());
    ASSERT_TRUE(configuration.runtime->dataset.has_value());
    EXPECT_EQ(*configuration.runtime->dataset, "remesh-sample");
    ASSERT_TRUE(configuration.benchmarks.has_value());
    ASSERT_EQ(configuration.benchmarks->scenarios.size(), 1U);
    ASSERT_TRUE(configuration.benchmarks->scenarios.front().dataset.has_value());
    EXPECT_EQ(*configuration.benchmarks->scenarios.front().dataset, "remesh-sample");
    ASSERT_TRUE(configuration.telemetry.has_value());
    EXPECT_EQ(configuration.telemetry->metrics.size(), 1U);
}

TEST(RuntimeConfigSchema, LoadConfigurationRejectsUnknownDataset)
{
    TempDirectory temp;
    const auto config_path = write_file(
        temp.path / "config.yaml",
        R"(datasets:
  - id: first-dataset
    schema:
      id: ai-004.dataset
      version: 1
    kind: geometry.remesh
    tags: [geometry]
    source:
      generator: geometry_remesh
      mesh: input.obj
    outputs:
      mesh: output.obj
    remeshing:
      mode: uniform
    feature_preservation:
      lock_boundary_edges: true
      lock_feature_edges: false
      minimum_feature_angle_degrees: 25.0
    metrics:
      input:
        vertices: 4
        faces: 4
        edge_length:
          min: 0.2
          max: 0.6
          mean: 0.4
      output:
        vertices: 8
        faces: 8
        edge_length:
          min: 0.1
          max: 0.5
          mean: 0.3
    statistics:
      iterations: 2
      max_error: 0.02
      min_edge_length: 0.1
      max_edge_length: 0.5
      max_surface_deviation: 0.01
      mean_surface_deviation: 0.008
      rms_surface_deviation: 0.009
runtime:
  schema:
    id: ai-004.runtime
    version: 1
  dataset: missing-dataset
  hot_reload:
    enabled: false
)"sv);

    auto result = load_configuration(config_path);
    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().code(), RuntimeError::configuration_validation_error);
    EXPECT_NE(result.error().message().find("missing-dataset"), std::string::npos);
}
