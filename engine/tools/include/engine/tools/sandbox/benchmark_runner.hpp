#pragma once

#include "engine/tools/api.hpp"
#include "engine/tools/sandbox/experiment_sandbox.hpp"

#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace engine::tools::sandbox
{
    /**
     * @brief Execute the prototyping harness in headless mode for benchmark capture.
     */
    class ENGINE_TOOLS_API PrototypeHarnessBenchmarkRunner
    {
    public:
        PrototypeHarnessBenchmarkRunner(std::vector<std::string> command_prefix,
                                        std::filesystem::path summary_directory);

        [[nodiscard]] SandboxBenchmarkResult run(const SandboxPreferences& preferences) const;

    private:
        [[nodiscard]] std::filesystem::path make_summary_path() const;
        [[nodiscard]] static std::string format_timestep(float timestep);
        [[nodiscard]] static std::string quote_argument(const std::string& argument);
        [[nodiscard]] static int execute_command(const std::vector<std::string>& command);
        [[nodiscard]] static std::optional<SandboxBenchmarkResult> parse_summary(
            const std::filesystem::path& path);

        std::vector<std::string> command_prefix_{};
        std::filesystem::path summary_directory_{};
    };

    /**
     * @brief Execute comparative benchmarks through the CC-310 orchestration script.
     */
    class ENGINE_TOOLS_API ComparativeBenchmarkRunner
    {
    public:
        ComparativeBenchmarkRunner(std::vector<std::string> command_prefix,
                                    std::filesystem::path working_directory);
        virtual ~ComparativeBenchmarkRunner();

        [[nodiscard]] virtual SandboxBenchmarkResult run(const BenchmarkScenarioDescriptor& scenario,
                                                         const SandboxPreferences& preferences) const;

    protected:
        [[nodiscard]] static std::string quote_argument(const std::string& argument);
        [[nodiscard]] static int execute_command(const std::vector<std::string>& command);

    private:
        [[nodiscard]] std::filesystem::path make_run_directory() const;
        [[nodiscard]] static std::string sanitise_filename_component(std::time_t timestamp);
        [[nodiscard]] static std::string escape_json(std::string_view text);
        [[nodiscard]] static std::string to_lower(std::string_view text);
        [[nodiscard]] bool write_configuration(const BenchmarkScenarioDescriptor& scenario,
                                               const std::filesystem::path& config_path,
                                               const std::filesystem::path& output_directory) const;
        [[nodiscard]] SandboxBenchmarkResult build_failure_result(std::string headline,
                                                                  std::string details) const;
        [[nodiscard]] static std::optional<SandboxBenchmarkResult> parse_summary(
            const std::filesystem::path& summary_path,
            const std::filesystem::path& table_path,
            const BenchmarkScenarioDescriptor& scenario,
            const SandboxPreferences& preferences);

        std::vector<std::string> command_prefix_{};
        std::filesystem::path working_directory_{};
    };
}

