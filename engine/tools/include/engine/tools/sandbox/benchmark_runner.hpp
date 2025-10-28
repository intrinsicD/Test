#pragma once

#include "engine/tools/api.hpp"
#include "engine/tools/sandbox/experiment_sandbox.hpp"

#include <filesystem>
#include <optional>
#include <string>
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
}

