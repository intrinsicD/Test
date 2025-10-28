#include "engine/tools/sandbox/benchmark_runner.hpp"

#include <gtest/gtest.h>

#include <array>
#include <filesystem>
#include <fstream>
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
        std::filesystem::create_directories(base);
        return base;
    }

    void write_file(const std::filesystem::path& path, std::string_view content)
    {
        std::ofstream stream(path, std::ios::trunc);
        ASSERT_TRUE(stream.is_open());
        stream << content;
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
               "args = parser.parse_args()\n"
               "payload = {\n"
               "    'dataset': 'geometry-baseline',\n"
               "    'rendering_preset': 'research',\n"
               "    'shading_mode': 'Forward',\n"
               "    'frames': args.frames,\n"
               "    'timestep_seconds': args.dt,\n"
               "}\n"
               "with open(args.summary_json, 'w', encoding='utf-8') as handle:\n"
               "    json.dump(payload, handle)\n");

    PrototypeHarnessBenchmarkRunner runner({*interpreter, script_path.string()}, temp_dir);

    SandboxPreferences preferences{};
    preferences.benchmark_frames = 240;
    preferences.benchmark_timestep = 1.0F / 120.0F;

    const auto result = runner.run(preferences);
    SCOPED_TRACE(result.details);
    EXPECT_TRUE(result.success);
    EXPECT_EQ(result.headline, "Benchmark succeeded");
    EXPECT_NE(result.details.find("frames=240"), std::string::npos);
    EXPECT_NE(result.details.find("dt=0.008333"), std::string::npos);
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
