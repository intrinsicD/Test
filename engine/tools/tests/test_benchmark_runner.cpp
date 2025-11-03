#include "engine/tools/sandbox/benchmark_runner.hpp"

#include <gtest/gtest.h>

#include <array>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <optional>

using namespace engine::tools::sandbox;

namespace
{
    std::optional<std::string> find_python_interpreter()
    {
        constexpr std::array<const char*, 3> candidates{"python3", "python", "py"};
#ifdef _WIN32
        constexpr const char* redirect = " > NUL 2>&1";
#else
        constexpr const char* redirect = " > /dev/null 2>&1";
#endif
        for (const auto* candidate : candidates)
        {
            std::string command = candidate;
            command += " --version";
            command += redirect;
            const int code = std::system(command.c_str());
            if (code == 0)
            {
                return std::string{candidate};
            }
        }
        return std::nullopt;
    }

    std::filesystem::path make_temp_directory(const std::string& name)
    {
        auto base = std::filesystem::temp_directory_path();
        base /= name;
        // Clean out any existing files from previous test runs
        if (std::filesystem::exists(base))
        {
            std::filesystem::remove_all(base);
        }
        std::filesystem::create_directories(base);
        return base;
    }

    void write_file(const std::filesystem::path& path, std::string_view content)
    {
        std::ofstream stream(path, std::ios::trunc);
        ASSERT_TRUE(stream.is_open());
        stream << content;
    }

    std::filesystem::path project_root()
    {
        auto source = std::filesystem::absolute(std::filesystem::path(__FILE__));
        return source.parent_path().parent_path().parent_path();
    }

    std::filesystem::path create_metric_emitter(const std::filesystem::path& path)
    {
        write_file(path,
                   "import json, pathlib, sys\n"
                   "target = pathlib.Path(sys.argv[1])\n"
                   "value = float(sys.argv[2])\n"
                   "target.parent.mkdir(parents=True, exist_ok=True)\n"
                   "target.write_text(json.dumps({'metrics': {'fps': value}}, indent=2), encoding='utf-8')\n");
        return path;
    }
}

TEST(PrototypeHarnessBenchmarkRunner, ExecutesSuccessfulBenchmark)
{
    const auto interpreter = find_python_interpreter();
    if (!interpreter)
    {
        GTEST_SKIP() << "Python interpreter not available";
    }

    const auto temp_dir = make_temp_directory("sandbox_runner_success");
    const auto script_path = temp_dir / "mock_harness.py";
    write_file(script_path,
               "import argparse\n"
               "import json\n"
               "parser = argparse.ArgumentParser()\n"
               "parser.add_argument('--frames', type=int, required=True)\n"
               "parser.add_argument('--dt', type=float, required=True)\n"
               "parser.add_argument('--summary-json', required=True)\n"
               "parser.add_argument('--dataset')\n"
               "parser.add_argument('--rendering-preset')\n"
               "parser.add_argument('--shading-mode')\n"
               "parser.add_argument('--runtime-profile')\n"
               "parser.add_argument('--resolution-width', type=int)\n"
               "parser.add_argument('--resolution-height', type=int)\n"
               "parser.add_argument('--overlay', action='append', default=[])\n"
               "args = parser.parse_args()\n"
               "payload = {\n"
               "    'scenario': 'geometry-baseline',\n"
               "    'dataset': args.dataset,\n"
               "    'rendering_preset': args.rendering_preset,\n"
               "    'shading_mode': args.shading_mode,\n"
               "    'runtime_profile': args.runtime_profile,\n"
               "    'resolution_width': args.resolution_width,\n"
               "    'resolution_height': args.resolution_height,\n"
               "    'overlays': args.overlay,\n"
               "    'frames': args.frames,\n"
               "    'timestep_seconds': args.dt,\n"
               "    'average_tick_ms': 8.3,\n"
               "    'run_index': 1,\n"
               "    'run_count': 3,\n"
               "    'dispatch_order': ['runtime.dispatch'],\n"
               "    'dispatch_durations_ms': [0.53],\n"
               "    'telemetry_outputs': [\n"
               "        {'kind': 'json', 'path': 'out/benchmark.json'}\n"
               "    ],\n"
               "}\n"
               "with open(args.summary_json, 'w', encoding='utf-8') as handle:\n"
               "    json.dump(payload, handle)\n");

    PrototypeHarnessBenchmarkRunner runner({*interpreter, script_path.string()}, temp_dir);

    SandboxPreferences preferences{};
    preferences.benchmark_frames = 240;
    preferences.benchmark_timestep = 1.0F / 120.0F;
    preferences.selected_dataset = "geometry-baseline";
    preferences.selected_algorithm_variant = "baseline";
    preferences.selected_preset = "research";
    preferences.shading_mode = "forward";
    preferences.overlays["normals"] = true;
    preferences.overlays["uv"] = false;
    preferences.resolution_width = 1920;
    preferences.resolution_height = 1080;

    const auto result = runner.run(preferences);
    SCOPED_TRACE(result.details);
    EXPECT_TRUE(result.success);
    EXPECT_EQ(result.headline, "Benchmark succeeded");
    EXPECT_NE(result.details.find("scenario=geometry-baseline"), std::string::npos);
    EXPECT_NE(result.details.find("runtime=baseline"), std::string::npos);
    EXPECT_NE(result.details.find("frames=240"), std::string::npos);
    EXPECT_NE(result.details.find("dt=0.008333"), std::string::npos);
    EXPECT_NE(result.details.find("avg_ms=8.300"), std::string::npos);
    EXPECT_NE(result.details.find("resolution=1920x1080"), std::string::npos);
    EXPECT_NE(result.details.find("run=1/3"), std::string::npos);

    std::filesystem::path summary_path;
    for (const auto& entry : std::filesystem::directory_iterator(temp_dir))
    {
        if (entry.path().extension() == ".json")
        {
            summary_path = entry.path();
            break;
        }
    }
    ASSERT_FALSE(summary_path.empty());
    std::ifstream summary_stream(summary_path);
    ASSERT_TRUE(summary_stream.is_open());
    const std::string summary_content{std::istreambuf_iterator<char>{summary_stream}, std::istreambuf_iterator<char>{}};
    EXPECT_NE(summary_content.find("\"scenario\": \"geometry-baseline\""), std::string::npos);
    EXPECT_NE(summary_content.find("normals=1"), std::string::npos);
    EXPECT_NE(summary_content.find("\"runtime_profile\": \"baseline\""), std::string::npos);
}

TEST(PrototypeHarnessBenchmarkRunner, ReportsFailure)
{
    const auto interpreter = find_python_interpreter();
    if (!interpreter)
    {
        GTEST_SKIP() << "Python interpreter not available";
    }

    const auto temp_dir = make_temp_directory("sandbox_runner_failure");
    const auto script_path = temp_dir / "failing_harness.py";
    write_file(script_path,
               "import sys\n"
               "sys.exit(3)\n");

    PrototypeHarnessBenchmarkRunner runner({*interpreter, script_path.string()}, temp_dir);

    SandboxPreferences preferences{};
    preferences.benchmark_frames = 60;
    preferences.benchmark_timestep = 1.0F / 60.0F;

    const auto result = runner.run(preferences);
    EXPECT_FALSE(result.success);
    EXPECT_NE(result.headline.find("Benchmark failed"), std::string::npos);
}

TEST(ComparativeBenchmarkRunner, ExecutesComparativeScenario)
{
    const auto interpreter = find_python_interpreter();
    if (!interpreter)
    {
        GTEST_SKIP() << "Python interpreter not available";
    }

    const auto orchestrator = project_root() / "scripts/benchmarks/run_comparative_benchmarks.py";
    if (!std::filesystem::exists(orchestrator))
    {
        GTEST_SKIP() << "Comparative benchmark orchestrator not available";
    }

    const auto temp_dir = make_temp_directory("comparative_runner_success");
    const auto emitter_path = create_metric_emitter(temp_dir / "emit_metrics.py");

    ComparativeBenchmarkRunner runner({*interpreter, orchestrator.string()}, temp_dir);

    BenchmarkScenarioDescriptor scenario{};
    scenario.identifier = "remesh-baseline";
    scenario.name = "remesh-baseline";
    scenario.dataset = "demo";
    scenario.rendering_preset = "research";
    scenario.runtime_profile = "baseline";
    scenario.engine.command = {*interpreter, emitter_path.string(), "{output_path}", "120.0"};
    scenario.engine.output = "{output_dir}/{scenario}_engine.json";
    scenario.reference.command = {*interpreter, emitter_path.string(), "{output_path}", "110.0"};
    scenario.reference.output = "{output_dir}/{scenario}_reference.json";
    BenchmarkMetricDescriptor metric{};
    metric.name = "fps";
    metric.higher_is_better = true;
    metric.threshold.mode = "relative";
    metric.threshold.limit = 0.25;
    scenario.metrics.push_back(metric);

    SandboxPreferences preferences{};
    preferences.selected_dataset = "demo";
    preferences.selected_preset = "research";
    preferences.selected_algorithm_variant = "baseline";

    const auto result = runner.run(scenario, preferences);
    SCOPED_TRACE(result.details);
    EXPECT_TRUE(result.success);
    EXPECT_NE(result.headline.find("succeeded"), std::string::npos);
    EXPECT_NE(result.details.find("PASS"), std::string::npos);
}

TEST(ComparativeBenchmarkRunner, ReportsRegression)
{
    const auto interpreter = find_python_interpreter();
    if (!interpreter)
    {
        GTEST_SKIP() << "Python interpreter not available";
    }

    const auto orchestrator = project_root() / "scripts/benchmarks/run_comparative_benchmarks.py";
    if (!std::filesystem::exists(orchestrator))
    {
        GTEST_SKIP() << "Comparative benchmark orchestrator not available";
    }

    const auto temp_dir = make_temp_directory("comparative_runner_failure");
    const auto emitter_path = create_metric_emitter(temp_dir / "emit_metrics.py");

    ComparativeBenchmarkRunner runner({*interpreter, orchestrator.string()}, temp_dir);

    BenchmarkScenarioDescriptor scenario{};
    scenario.identifier = "regression";
    scenario.name = "regression";
    scenario.dataset = "demo";
    scenario.engine.command = {*interpreter, emitter_path.string(), "{output_path}", "90.0"};
    scenario.engine.output = "{output_dir}/{scenario}_engine.json";
    scenario.reference.command = {*interpreter, emitter_path.string(), "{output_path}", "110.0"};
    scenario.reference.output = "{output_dir}/{scenario}_reference.json";
    BenchmarkMetricDescriptor metric{};
    metric.name = "fps";
    metric.higher_is_better = true;
    metric.threshold.mode = "relative";
    metric.threshold.limit = 0.05;
    scenario.metrics.push_back(metric);

    SandboxPreferences preferences{};
    preferences.selected_dataset = "demo";

    const auto result = runner.run(scenario, preferences);
    SCOPED_TRACE(result.details);
    EXPECT_FALSE(result.success);
    EXPECT_NE(result.headline.find("failed"), std::string::npos);
    EXPECT_NE(result.details.find("FAIL"), std::string::npos);
}
