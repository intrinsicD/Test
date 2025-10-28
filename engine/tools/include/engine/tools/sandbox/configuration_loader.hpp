#pragma once

#include "engine/tools/api.hpp"
#include "engine/tools/sandbox/experiment_sandbox.hpp"

#include <filesystem>
#include <string_view>

namespace engine::tools::sandbox
{
    /**
     * \brief Parse an experiment configuration summary from a JSON buffer.
     *
     * The JSON payload must match the schema produced by
     * `scripts/prototyping/run_prototype_harness.py --describe-json`.
     */
    [[nodiscard]] ENGINE_TOOLS_API ExperimentConfigurationSummary load_summary_from_json(
        std::string_view json_buffer);

    /**
     * \brief Load an experiment configuration summary from a JSON file on disk.
     */
    [[nodiscard]] ENGINE_TOOLS_API ExperimentConfigurationSummary load_summary_from_json(
        const std::filesystem::path& path);
}
