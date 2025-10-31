#include "engine/tools/sandbox/experiment_sandbox.hpp"
#include "engine/tools/sandbox/benchmark_runner.hpp"

#include <imgui.h>

#include <algorithm>
#include <cctype>
#include <charconv>
#include <cmath>
#include <iomanip>
#include <filesystem>
#include <fstream>
#include <limits>
#include <sstream>
#include <system_error>

namespace engine::tools::sandbox
{
    namespace
    {
        constexpr std::size_t kMaxTelemetrySamples = 256;

        std::string to_lower(std::string_view input)
        {
            std::string output{input};
            std::transform(output.begin(), output.end(), output.begin(), [](unsigned char ch) {
                return static_cast<char>(std::tolower(ch));
            });
            return output;
        }

        TelemetrySeries normalise_telemetry_series(const TelemetrySeries& input)
        {
            TelemetrySeries series = input;
            if (series.samples.empty())
            {
                return series;
            }

            if (series.samples.size() > kMaxTelemetrySamples)
            {
                const std::size_t total = series.samples.size();
                const std::size_t target = std::min(kMaxTelemetrySamples, total);
                std::vector<float> downsampled;
                downsampled.reserve(target);

                if (target <= 1)
                {
                    downsampled.push_back(series.samples.front());
                }
                else
                {
                    const double step = static_cast<double>(total - 1)
                        / static_cast<double>(target - 1);
                    for (std::size_t i = 0; i < target; ++i)
                    {
                        const double position = step * static_cast<double>(i);
                        std::size_t index = static_cast<std::size_t>(std::llround(position));
                        if (index >= total)
                        {
                            index = total - 1;
                        }

                        downsampled.push_back(series.samples[index]);
                        if (index == total - 1)
                        {
                            break;
                        }
                    }

                    if (downsampled.back() != series.samples.back())
                    {
                        downsampled.push_back(series.samples.back());
                    }
                }

                series.samples = std::move(downsampled);
            }

            const auto [min_it, max_it] =
                std::minmax_element(series.samples.begin(), series.samples.end());
            series.minimum = *min_it;
            series.maximum = *max_it;
            return series;
        }

        std::string trim(std::string_view input)
        {
            std::size_t begin = 0;
            std::size_t end = input.size();
            while (begin < end && std::isspace(static_cast<unsigned char>(input[begin])))
            {
                ++begin;
            }
            while (end > begin && std::isspace(static_cast<unsigned char>(input[end - 1])))
            {
                --end;
            }
            return std::string{input.substr(begin, end - begin)};
        }

        bool parse_bool(std::string_view value)
        {
            const auto lowered = to_lower(value);
            return lowered == "1" || lowered == "true" || lowered == "yes" || lowered == "on";
        }

        template <typename T>
        bool parse_numeric(std::string_view text, T& out_value)
        {
            auto begin = text.data();
            auto end = text.data() + text.size();
            auto result = std::from_chars(begin, end, out_value);
            return result.ec == std::errc{};
        }

        std::string format_uintmax(std::uintmax_t value)
        {
            std::ostringstream stream;
            stream << value;
            return stream.str();
        }

        std::string format_double(double value)
        {
            if (std::isfinite(value))
            {
                const long long rounded = std::llround(value);
                if (std::abs(value - static_cast<double>(rounded)) < 1e-6)
                {
                    return std::to_string(rounded);
                }
            }

            std::ostringstream stream;
            stream.setf(std::ios::fixed, std::ios::floatfield);
            stream << std::setprecision(3) << value;
            return stream.str();
        }

        void render_property_block(std::string_view heading,
                                   const std::vector<std::pair<std::string, std::string>>& entries)
        {
            if (entries.empty())
            {
                return;
            }

            ImGui::TextUnformatted(std::string{heading}.c_str());
            ImGui::Indent();
            for (const auto& [key, value] : entries)
            {
                ImGui::BulletText("%s: %s", key.c_str(), value.c_str());
            }
            ImGui::Unindent();
        }

        void render_numeric_property_block(std::string_view heading,
                                           const std::vector<std::pair<std::string, double>>& entries)
        {
            if (entries.empty())
            {
                return;
            }

            ImGui::TextUnformatted(std::string{heading}.c_str());
            ImGui::Indent();
            for (const auto& [key, value] : entries)
            {
                ImGui::BulletText("%s: %s", key.c_str(), format_double(value).c_str());
            }
            ImGui::Unindent();
        }

        void render_dataset_asset(const DatasetAssetDescriptor& asset)
        {
            const ImVec4 success_colour{0.2F, 0.8F, 0.2F, 1.0F};
            const ImVec4 failure_colour{0.9F, 0.2F, 0.2F, 1.0F};
            const bool verified = asset.verified;
            const ImVec4 colour = verified ? success_colour : failure_colour;
            const char* status = verified ? "Verified" : "Verification failed";

            ImGui::TextColored(colour, "%s — %s", asset.role.c_str(), status);
            ImGui::Indent();

            const std::string& resolved = asset.resolved_path.empty() ? asset.path : asset.resolved_path;
            if (!resolved.empty())
            {
                ImGui::TextWrapped("Path: %s", resolved.c_str());
            }

            if (!verified)
            {
                ImGui::Text("Exists: %s", asset.exists ? "yes" : "no");

                if (asset.expected_size_bytes)
                {
                    const std::string expected = format_uintmax(*asset.expected_size_bytes);
                    if (asset.actual_size_bytes)
                    {
                        const std::string actual = format_uintmax(*asset.actual_size_bytes);
                        ImGui::Text("Size (expected/actual): %s / %s bytes", expected.c_str(), actual.c_str());
                    }
                    else
                    {
                        ImGui::Text("Expected size: %s bytes", expected.c_str());
                    }
                }
                else if (asset.actual_size_bytes)
                {
                    const std::string actual = format_uintmax(*asset.actual_size_bytes);
                    ImGui::Text("Actual size: %s bytes", actual.c_str());
                }

                if (asset.expected_sha256)
                {
                    ImGui::Text("Expected sha256: %s", asset.expected_sha256->c_str());
                }
                if (asset.actual_sha256)
                {
                    ImGui::Text("Actual sha256: %s", asset.actual_sha256->c_str());
                }
            }

            if (asset.message && !asset.message->empty())
            {
                ImGui::TextWrapped("Note: %s", asset.message->c_str());
            }

            ImGui::Unindent();
        }
    } // namespace

    ExperimentSandbox::ExperimentSandbox()
    {
        dataset_filter_buffer_.fill('\0');
    }

    void ExperimentSandbox::set_configuration(const ExperimentConfigurationSummary& summary)
    {
        const SandboxPreferences previous_preferences = preferences_;

        summary_ = summary;
        dataset_lookup_.clear();
        preset_lookup_.clear();
        algorithm_lookup_.clear();
        case_study_lookup_.clear();
        last_benchmark_result_.reset();

        for (std::size_t index = 0; index < summary_.datasets.size(); ++index)
        {
            dataset_lookup_.emplace(summary_.datasets[index].identifier, index);
        }

        for (std::size_t index = 0; index < summary_.rendering_presets.size(); ++index)
        {
            preset_lookup_.emplace(summary_.rendering_presets[index].identifier, index);
        }

        for (std::size_t index = 0; index < summary_.algorithm_variants.size(); ++index)
        {
            algorithm_lookup_.emplace(summary_.algorithm_variants[index].identifier, index);
        }

        for (std::size_t index = 0; index < summary_.case_studies.size(); ++index)
        {
            case_study_lookup_.emplace(summary_.case_studies[index].identifier, index);
        }

        if (summary_.selected_dataset)
        {
            preferences_.selected_dataset = *summary_.selected_dataset;
        }

        if (summary_.selected_rendering_preset)
        {
            preferences_.selected_preset = *summary_.selected_rendering_preset;
        }

        if (summary_.selected_algorithm_variant)
        {
            preferences_.selected_algorithm_variant = *summary_.selected_algorithm_variant;
        }

        if (summary_.selected_case_study)
        {
            preferences_.selected_case_study = *summary_.selected_case_study;
        }

        ensure_selection_defaults();
        notify_preference_changes(previous_preferences);
    }

    void ExperimentSandbox::update_telemetry(const TelemetrySnapshot& telemetry)
    {
        TelemetrySnapshot filtered = telemetry;
        filtered.series.clear();
        filtered.series.reserve(telemetry.series.size());
        for (const auto& series : telemetry.series)
        {
            filtered.series.emplace_back(normalise_telemetry_series(series));
        }

        telemetry_ = std::move(filtered);
    }

    void ExperimentSandbox::set_callbacks(SandboxCallbacks callbacks)
    {
        callbacks_ = std::move(callbacks);

        const bool has_dataset_selection = selected_dataset_index_ >= 0
            && selected_dataset_index_ < static_cast<int>(summary_.datasets.size());
        if (callbacks_.on_dataset_selected && has_dataset_selection)
        {
            callbacks_.on_dataset_selected(preferences_.selected_dataset);
        }

        const bool has_preset_selection = selected_preset_index_ >= 0
            && selected_preset_index_ < static_cast<int>(summary_.rendering_presets.size());
        if (callbacks_.on_rendering_changed && has_preset_selection)
        {
            callbacks_.on_rendering_changed(preferences_);
        }

        const bool has_algorithm_selection = selected_algorithm_index_ >= 0
            && selected_algorithm_index_ < static_cast<int>(summary_.algorithm_variants.size());
        if (callbacks_.on_algorithm_selected && has_algorithm_selection)
        {
            callbacks_.on_algorithm_selected(preferences_.selected_algorithm_variant);
        }

        const bool has_case_selection = selected_case_study_index_ >= 0
            && selected_case_study_index_ < static_cast<int>(summary_.case_studies.size());
        if (callbacks_.on_case_study_selected && has_case_selection)
        {
            callbacks_.on_case_study_selected(preferences_.selected_case_study);
        }
    }

    void ExperimentSandbox::set_comparative_benchmark_runner(std::shared_ptr<ComparativeBenchmarkRunner> runner)
    {
        comparative_runner_ = std::move(runner);
    }

    std::optional<SandboxBenchmarkResult> ExperimentSandbox::run_active_benchmark()
    {
        if (callbacks_.on_run_benchmark)
        {
            return callbacks_.on_run_benchmark(preferences_);
        }

        if (comparative_runner_ && summary_.benchmarks)
        {
            if (const auto* scenario = find_active_benchmark_scenario())
            {
                return comparative_runner_->run(*scenario, preferences_);
            }

            SandboxBenchmarkResult result{};
            result.success = false;
            result.headline = "No matching benchmark scenario";
            result.details = "The current selection does not match any comparative benchmark scenario.";
            return result;
        }

        return std::nullopt;
    }

    void ExperimentSandbox::render()
    {
        if (!ImGui::Begin("Experiment Sandbox"))
        {
            ImGui::End();
            return;
        }

        ImGui::Columns(2, nullptr, false);
        render_dataset_panel();
        ImGui::NextColumn();
        render_details_panel();
        ImGui::Columns(1);

        ImGui::End();
    }

    const SandboxPreferences& ExperimentSandbox::preferences() const noexcept
    {
        return preferences_;
    }

    const TelemetrySnapshot& ExperimentSandbox::telemetry_snapshot() const noexcept
    {
        return telemetry_;
    }

    void ExperimentSandbox::set_preferences(const SandboxPreferences& preferences)
    {
        const SandboxPreferences previous_preferences = preferences_;
        preferences_ = preferences;
        ensure_selection_defaults();
        notify_preference_changes(previous_preferences);
    }

    bool ExperimentSandbox::select_algorithm_variant(std::string_view variant_identifier)
    {
        const std::string identifier{variant_identifier};
        const auto lookup = algorithm_lookup_.find(identifier);
        if (lookup == algorithm_lookup_.end())
        {
            return false;
        }

        const int index = static_cast<int>(lookup->second);
        if (selected_algorithm_index_ == index && preferences_.selected_algorithm_variant == identifier)
        {
            return true;
        }

        selected_algorithm_index_ = index;
        preferences_.selected_algorithm_variant = identifier;

        if (callbacks_.on_algorithm_selected)
        {
            callbacks_.on_algorithm_selected(preferences_.selected_algorithm_variant);
        }

        return true;
    }

    bool ExperimentSandbox::select_dataset(std::string_view dataset_identifier)
    {
        const std::string identifier{dataset_identifier};
        const auto lookup = dataset_lookup_.find(identifier);
        if (lookup == dataset_lookup_.end())
        {
            return false;
        }

        const int index = static_cast<int>(lookup->second);
        if (selected_dataset_index_ == index && preferences_.selected_dataset == identifier)
        {
            return true;
        }

        selected_dataset_index_ = index;
        preferences_.selected_dataset = identifier;

        if (callbacks_.on_dataset_selected)
        {
            callbacks_.on_dataset_selected(preferences_.selected_dataset);
        }

        return true;
    }

    bool ExperimentSandbox::select_rendering_preset(std::string_view preset_identifier)
    {
        const std::string identifier{preset_identifier};
        const auto lookup = preset_lookup_.find(identifier);
        if (lookup == preset_lookup_.end())
        {
            return false;
        }

        const int index = static_cast<int>(lookup->second);
        const bool changed = selected_preset_index_ != index
            || preferences_.selected_preset != identifier;

        selected_preset_index_ = index;
        preferences_.selected_preset = identifier;

        bool state_changed = changed;

        if (selected_preset_index_ >= 0
            && selected_preset_index_ < static_cast<int>(summary_.rendering_presets.size()))
        {
            const auto& preset = summary_.rendering_presets[static_cast<std::size_t>(selected_preset_index_)];

            if (!preset.shading_modes.empty()
                && std::find(preset.shading_modes.begin(), preset.shading_modes.end(), preferences_.shading_mode)
                    == preset.shading_modes.end())
            {
                preferences_.shading_mode = preset.shading_modes.front();
                state_changed = true;
            }
            else if (preset.shading_modes.empty())
            {
                preferences_.shading_mode.clear();
                state_changed = true;
            }

            if (sync_overlay_preferences())
            {
                state_changed = true;
            }
        }

        if (callbacks_.on_rendering_changed && state_changed)
        {
            callbacks_.on_rendering_changed(preferences_);
        }

        return true;
    }

    bool ExperimentSandbox::set_shading_mode(std::string_view shading_mode)
    {
        if (selected_preset_index_ < 0
            || selected_preset_index_ >= static_cast<int>(summary_.rendering_presets.size()))
        {
            return false;
        }

        const auto& preset = summary_.rendering_presets[static_cast<std::size_t>(selected_preset_index_)];
        const auto it = std::find(preset.shading_modes.begin(), preset.shading_modes.end(), shading_mode);
        if (it == preset.shading_modes.end())
        {
            return false;
        }

        if (preferences_.shading_mode == shading_mode)
        {
            return true;
        }

        preferences_.shading_mode = std::string{shading_mode};

        if (callbacks_.on_rendering_changed)
        {
            callbacks_.on_rendering_changed(preferences_);
        }

        return true;
    }

    bool ExperimentSandbox::set_overlay_enabled(std::string_view overlay_key, bool enabled)
    {
        if (selected_preset_index_ < 0
            || selected_preset_index_ >= static_cast<int>(summary_.rendering_presets.size()))
        {
            return false;
        }

        static_cast<void>(sync_overlay_preferences());

        const auto& preset = summary_.rendering_presets[static_cast<std::size_t>(selected_preset_index_)];
        const auto overlay_it = std::find_if(preset.overlays.begin(), preset.overlays.end(),
                                             [&](const OverlayDescriptor& descriptor) {
                                                 return descriptor.key == overlay_key;
                                             });
        if (overlay_it == preset.overlays.end())
        {
            return false;
        }

        auto& stored = preferences_.overlays[overlay_it->key];
        if (stored == enabled)
        {
            return true;
        }

        stored = enabled;

        if (callbacks_.on_rendering_changed)
        {
            callbacks_.on_rendering_changed(preferences_);
        }

        return true;
    }

    bool ExperimentSandbox::request_case_study(std::string_view case_study_identifier)
    {
        const std::string identifier{case_study_identifier};
        const auto lookup = case_study_lookup_.find(identifier);
        if (lookup == case_study_lookup_.end())
        {
            return false;
        }

        const int index = static_cast<int>(lookup->second);
        const auto& descriptor = summary_.case_studies[static_cast<std::size_t>(index)];

        const bool already_selected = selected_case_study_index_ == index
            && preferences_.selected_case_study == identifier;

        if (!already_selected)
        {
            const SandboxPreferences previous = preferences_;
            selected_case_study_index_ = index;
            preferences_.selected_case_study = identifier;
            notify_preference_changes(previous);
        }

        if (callbacks_.on_case_study_requested)
        {
            callbacks_.on_case_study_requested(descriptor);
        }

        return true;
    }

    bool ExperimentSandbox::load_preferences(const std::filesystem::path& path)
    {
        std::ifstream stream(path);
        if (!stream.is_open())
        {
            return false;
        }

        SandboxPreferences loaded = preferences_;
        loaded.overlays.clear();

        std::string line;
        while (std::getline(stream, line))
        {
            const auto trimmed = trim(line);
            if (trimmed.empty() || trimmed.front() == '#')
            {
                continue;
            }

            const auto equals = trimmed.find('=');
            if (equals == std::string::npos)
            {
                continue;
            }

            const std::string key = trim(trimmed.substr(0, equals));
            const std::string value = trim(trimmed.substr(equals + 1));

            if (key == "selected_dataset")
            {
                loaded.selected_dataset = value;
            }
            else if (key == "selected_preset")
            {
                loaded.selected_preset = value;
            }
            else if (key == "selected_algorithm_variant")
            {
                loaded.selected_algorithm_variant = value;
            }
            else if (key == "selected_case_study")
            {
                loaded.selected_case_study = value;
            }
            else if (key == "shading_mode")
            {
                loaded.shading_mode = value;
            }
            else if (key == "benchmark_frames")
            {
                int frames = loaded.benchmark_frames;
                if (parse_numeric(value, frames) && frames > 0)
                {
                    loaded.benchmark_frames = frames;
                }
            }
            else if (key == "benchmark_timestep")
            {
                float timestep = loaded.benchmark_timestep;
                if (parse_numeric(value, timestep) && timestep > 0.0F)
                {
                    loaded.benchmark_timestep = timestep;
                }
            }
            else if (key == "resolution_width")
            {
                int width = loaded.resolution_width;
                if (parse_numeric(value, width))
                {
                    loaded.resolution_width = std::max(width, 0);
                }
            }
            else if (key == "resolution_height")
            {
                int height = loaded.resolution_height;
                if (parse_numeric(value, height))
                {
                    loaded.resolution_height = std::max(height, 0);
                }
            }
            else if (key.rfind("overlay.", 0) == 0U)
            {
                const auto overlay_key = key.substr(8);
                loaded.overlays[overlay_key] = parse_bool(value);
            }
        }

        set_preferences(loaded);
        return true;
    }

    bool ExperimentSandbox::save_preferences(const std::filesystem::path& path) const
    {
        if (!path.empty() && path.has_parent_path())
        {
            std::error_code ec;
            std::filesystem::create_directories(path.parent_path(), ec);
            if (ec)
            {
                return false;
            }
        }

        std::ofstream stream(path, std::ios::trunc);
        if (!stream.is_open())
        {
            return false;
        }

        stream << "# Experiment sandbox preferences\n";
        stream << "selected_dataset=" << preferences_.selected_dataset << '\n';
        stream << "selected_preset=" << preferences_.selected_preset << '\n';
        stream << "selected_algorithm_variant=" << preferences_.selected_algorithm_variant << '\n';
        stream << "selected_case_study=" << preferences_.selected_case_study << '\n';
        stream << "shading_mode=" << preferences_.shading_mode << '\n';
        stream << "resolution_width=" << preferences_.resolution_width << '\n';
        stream << "resolution_height=" << preferences_.resolution_height << '\n';
        stream << "benchmark_frames=" << preferences_.benchmark_frames << '\n';
        stream << "benchmark_timestep=" << preferences_.benchmark_timestep << '\n';

        for (const auto& [key, value] : preferences_.overlays)
        {
            stream << "overlay." << key << '=' << (value ? "1" : "0") << '\n';
        }

        return stream.good();
    }

    bool ExperimentSandbox::load_layout(const std::filesystem::path& path)
    {
        if (!std::filesystem::exists(path))
        {
            return false;
        }
        ImGui::LoadIniSettingsFromDisk(path.string().c_str());
        return true;
    }

    bool ExperimentSandbox::save_layout(const std::filesystem::path& path) const
    {
        if (!path.empty() && path.has_parent_path())
        {
            std::error_code ec;
            std::filesystem::create_directories(path.parent_path(), ec);
            if (ec)
            {
                return false;
            }
        }
        ImGui::SaveIniSettingsToDisk(path.string().c_str());
        return std::filesystem::exists(path);
    }

    void ExperimentSandbox::ensure_selection_defaults()
    {
        selected_dataset_index_ = -1;
        if (!summary_.datasets.empty())
        {
            auto it = dataset_lookup_.find(preferences_.selected_dataset);
            if (it != dataset_lookup_.end())
            {
                selected_dataset_index_ = static_cast<int>(it->second);
            }
            else
            {
                selected_dataset_index_ = 0;
                preferences_.selected_dataset = summary_.datasets.front().identifier;
            }
        }
        else
        {
            preferences_.selected_dataset.clear();
        }

        selected_preset_index_ = -1;
        if (!summary_.rendering_presets.empty())
        {
            auto it = preset_lookup_.find(preferences_.selected_preset);
            if (it != preset_lookup_.end())
            {
                selected_preset_index_ = static_cast<int>(it->second);
            }
            else
            {
                selected_preset_index_ = 0;
                preferences_.selected_preset = summary_.rendering_presets.front().identifier;
            }

            const auto& preset = summary_.rendering_presets[static_cast<std::size_t>(selected_preset_index_)];
            if (!preset.shading_modes.empty())
            {
                if (std::find(preset.shading_modes.begin(), preset.shading_modes.end(), preferences_.shading_mode) ==
                    preset.shading_modes.end())
                {
                    preferences_.shading_mode = preset.shading_modes.front();
                }
            }
            else
            {
                preferences_.shading_mode.clear();
            }
        }
        else
        {
            preferences_.selected_preset.clear();
            preferences_.shading_mode.clear();
            preferences_.overlays.clear();
        }

        selected_algorithm_index_ = -1;
        if (!summary_.algorithm_variants.empty())
        {
            auto it = algorithm_lookup_.find(preferences_.selected_algorithm_variant);
            if (it != algorithm_lookup_.end())
            {
                selected_algorithm_index_ = static_cast<int>(it->second);
            }
            else
            {
                selected_algorithm_index_ = 0;
                preferences_.selected_algorithm_variant = summary_.algorithm_variants.front().identifier;
            }
        }
        else
        {
            preferences_.selected_algorithm_variant.clear();
        }

        selected_case_study_index_ = -1;
        if (!summary_.case_studies.empty())
        {
            auto it = case_study_lookup_.find(preferences_.selected_case_study);
            if (it != case_study_lookup_.end())
            {
                selected_case_study_index_ = static_cast<int>(it->second);
            }
            else
            {
                selected_case_study_index_ = 0;
                preferences_.selected_case_study = summary_.case_studies.front().identifier;
            }
        }
        else
        {
            preferences_.selected_case_study.clear();
        }

        static_cast<void>(sync_overlay_preferences());
    }

    void ExperimentSandbox::render_dataset_panel()
    {
        ImGui::TextUnformatted("Datasets");
        ImGui::Spacing();

        if (ImGui::InputTextWithHint("##DatasetFilter", "Filter datasets...", dataset_filter_buffer_.data(),
                                     dataset_filter_buffer_.size()))
        {
            dataset_filter_ = dataset_filter_buffer_.data();
            dataset_filter_lower_ = to_lower(dataset_filter_);
        }

        ImGui::Separator();

        if (summary_.datasets.empty())
        {
            ImGui::TextDisabled("No datasets available");
            return;
        }

        if (ImGui::BeginChild("DatasetList", ImVec2(0.0F, 0.0F), true))
        {
            for (std::size_t index = 0; index < summary_.datasets.size(); ++index)
            {
                const auto& dataset = summary_.datasets[index];
                const bool visible = matches_dataset_filter(dataset.identifier) ||
                    (!dataset.label.empty() && matches_dataset_filter(dataset.label));

                bool tag_match = false;
                if (!visible && !dataset.tags.empty())
                {
                    for (const auto& tag : dataset.tags)
                    {
                        if (matches_dataset_filter(tag))
                        {
                            tag_match = true;
                            break;
                        }
                    }
                }

                if (!visible && !tag_match)
                {
                    continue;
                }

                const bool selected = static_cast<int>(index) == selected_dataset_index_;
                std::string label = dataset.label.empty() ? dataset.identifier : dataset.label;
                label.append("##").append(dataset.identifier);
                if (ImGui::Selectable(label.c_str(), selected))
                {
                    static_cast<void>(select_dataset(dataset.identifier));
                }

                if (!dataset.tags.empty())
                {
                    ImGui::SameLine();
                    ImGui::TextDisabled("%s", dataset.kind.c_str());
                }
            }
            ImGui::EndChild();
        }
    }

    void ExperimentSandbox::render_details_panel()
    {
        render_runtime_summary();
        ImGui::Spacing();
        render_rendering_panel();
        ImGui::Spacing();
        render_benchmark_panel();
        ImGui::Spacing();
        render_telemetry_panel();
    }

    void ExperimentSandbox::render_rendering_panel()
    {
        ImGui::TextUnformatted("Rendering");
        ImGui::Separator();

        if (summary_.rendering_presets.empty())
        {
            ImGui::TextDisabled("No rendering presets available");
            return;
        }

        const auto& preset = summary_.rendering_presets[static_cast<std::size_t>(selected_preset_index_)];
        std::string preset_label = preset.label.empty() ? preset.identifier : preset.label;

        if (ImGui::BeginCombo("Preset", preset_label.c_str()))
        {
            for (std::size_t index = 0; index < summary_.rendering_presets.size(); ++index)
            {
                const auto& option = summary_.rendering_presets[index];
                const bool selected = static_cast<int>(index) == selected_preset_index_;
                std::string option_label = option.label.empty() ? option.identifier : option.label;
                if (ImGui::Selectable(option_label.c_str(), selected))
                {
                    static_cast<void>(select_rendering_preset(option.identifier));
                }
            }
            ImGui::EndCombo();
        }

        if (!preset.shading_modes.empty())
        {
            std::string shading_label = preferences_.shading_mode.empty() ? "<select>" : preferences_.shading_mode;
            if (ImGui::BeginCombo("Shading Mode", shading_label.c_str()))
            {
                for (const auto& mode : preset.shading_modes)
                {
                    const bool selected = (mode == preferences_.shading_mode);
                    if (ImGui::Selectable(mode.c_str(), selected))
                    {
                        static_cast<void>(set_shading_mode(mode));
                    }
                }
                ImGui::EndCombo();
            }
        }

        for (const auto& overlay : preset.overlays)
        {
            bool enabled = preferences_.overlays[overlay.key];
            std::string checkbox_label = overlay.label.empty() ? overlay.key : overlay.label;
            if (ImGui::Checkbox(checkbox_label.c_str(), &enabled))
            {
                if (!set_overlay_enabled(overlay.key, enabled))
                {
                    enabled = preferences_.overlays[overlay.key];
                }
            }
        }
    }

    void ExperimentSandbox::render_benchmark_panel()
    {
        ImGui::TextUnformatted("Benchmark");
        ImGui::Separator();

        int frames = preferences_.benchmark_frames;
        if (ImGui::InputInt("Frames", &frames, 60, 600))
        {
            preferences_.benchmark_frames = std::max(frames, 1);
        }

        float timestep = preferences_.benchmark_timestep;
        if (ImGui::InputFloat("Timestep (s)", &timestep, 0.001F, 0.01F, "%.6f"))
        {
            preferences_.benchmark_timestep = std::max(timestep, std::numeric_limits<float>::epsilon());
        }

        if (ImGui::Button("Run Benchmark"))
        {
            if (auto result = run_active_benchmark())
            {
                apply_benchmark_result(*result);
            }
        }

        if (!summary_.case_studies.empty())
        {
            ImGui::Spacing();
            ImGui::TextUnformatted("Case Studies");
            ImGui::Separator();
            ImGui::Indent();
            for (std::size_t index = 0; index < summary_.case_studies.size(); ++index)
            {
                const auto& descriptor = summary_.case_studies[index];
                const bool active = static_cast<int>(index) == selected_case_study_index_;
                const ImVec4 colour = active ? ImVec4(0.2F, 0.8F, 0.2F, 1.0F)
                                             : ImGui::GetStyleColorVec4(ImGuiCol_Text);
                ImGui::TextColored(colour, "%s", descriptor.label.empty() ? descriptor.identifier.c_str()
                                                                            : descriptor.label.c_str());
                ImGui::Indent();
                if (!descriptor.description.empty())
                {
                    ImGui::PushTextWrapPos(0.0F);
                    ImGui::TextUnformatted(descriptor.description.c_str());
                    ImGui::PopTextWrapPos();
                }
                if (!descriptor.tags.empty())
                {
                    ImGui::Text("Tags: %s", descriptor.tags.front().c_str());
                    for (std::size_t tag_index = 1; tag_index < descriptor.tags.size(); ++tag_index)
                    {
                        ImGui::SameLine();
                        ImGui::Text("%s", descriptor.tags[tag_index].c_str());
                    }
                }
                if (descriptor.default_dataset)
                {
                    ImGui::Text("Dataset: %s", descriptor.default_dataset->c_str());
                }
                if (descriptor.default_rendering_preset)
                {
                    ImGui::Text("Rendering preset: %s", descriptor.default_rendering_preset->c_str());
                }
                if (descriptor.default_runtime_profile)
                {
                    ImGui::Text("Runtime profile: %s", descriptor.default_runtime_profile->c_str());
                }
                if (descriptor.default_shading_mode)
                {
                    ImGui::Text("Shading: %s", descriptor.default_shading_mode->c_str());
                }
                if (descriptor.default_resolution_width && descriptor.default_resolution_height)
                {
                    ImGui::Text("Resolution: %dx%d", *descriptor.default_resolution_width,
                                *descriptor.default_resolution_height);
                }
                if (!descriptor.default_overlays.empty())
                {
                    ImGui::TextUnformatted("Overlays:");
                    ImGui::Indent();
                    for (const auto& [key, value] : descriptor.default_overlays)
                    {
                        ImGui::BulletText("%s: %s", key.c_str(), value ? "enabled" : "disabled");
                    }
                    ImGui::Unindent();
                }
                if (!descriptor.benchmark_scenarios.empty())
                {
                    ImGui::TextUnformatted("Scenarios:");
                    ImGui::Indent();
                    for (const auto& scenario_identifier : descriptor.benchmark_scenarios)
                    {
                        ImGui::BulletText("%s", scenario_identifier.c_str());
                    }
                    ImGui::Unindent();
                }
                if (!descriptor.config_absolute.empty())
                {
                    ImGui::TextDisabled("Config: %s", descriptor.config_absolute.c_str());
                }
                const std::string button_id = std::string{"Load##case_study_"} + descriptor.identifier;
                if (ImGui::Button(button_id.c_str()))
                {
                    static_cast<void>(request_case_study(descriptor.identifier));
                }
                ImGui::Spacing();
                ImGui::Unindent();
            }
            ImGui::Unindent();
        }

        if (summary_.benchmarks && !summary_.benchmarks->scenarios.empty())
        {
            ImGui::Spacing();
            ImGui::TextUnformatted("Scenarios");
            ImGui::Separator();
            ImGui::Indent();
            for (const auto& scenario : summary_.benchmarks->scenarios)
            {
                std::string headline = scenario.identifier;
                if (!scenario.name.empty() && scenario.name != scenario.identifier)
                {
                    headline += " — " + scenario.name;
                }
                ImGui::BulletText("%s", headline.c_str());
                ImGui::Indent();
                if (!scenario.dataset.empty())
                {
                    ImGui::Text("Dataset: %s", scenario.dataset.c_str());
                }
                if (!scenario.rendering_preset.empty())
                {
                    ImGui::Text("Rendering preset: %s", scenario.rendering_preset.c_str());
                }
                if (!scenario.runtime_profile.empty())
                {
                    ImGui::Text("Runtime profile: %s", scenario.runtime_profile.c_str());
                }
                if (!scenario.metrics.empty())
                {
                    ImGui::TextUnformatted("Metrics:");
                    ImGui::Indent();
                    for (const auto& metric : scenario.metrics)
                    {
                        const char* direction = metric.higher_is_better ? "higher is better" : "lower is better";
                        ImGui::BulletText("%s (%s, limit %.3f)", metric.name.c_str(), direction,
                                          metric.threshold.limit);
                    }
                    ImGui::Unindent();
                }
                ImGui::Unindent();
            }
            ImGui::Unindent();
        }

        if (last_benchmark_result_)
        {
            const auto& result = *last_benchmark_result_;
            const ImVec4 colour = result.success ? ImVec4(0.2F, 0.8F, 0.2F, 1.0F) : ImVec4(0.9F, 0.2F, 0.2F, 1.0F);
            ImGui::Spacing();
            ImGui::TextColored(colour, "%s", result.headline.c_str());
            if (!result.details.empty())
            {
                ImGui::PushTextWrapPos(0.0F);
                ImGui::TextUnformatted(result.details.c_str());
                ImGui::PopTextWrapPos();
            }
        }
    }

    void ExperimentSandbox::render_telemetry_panel()
    {
        ImGui::TextUnformatted("Telemetry");
        ImGui::Separator();

        ImGui::Text("FPS: %.2f", telemetry_.frames_per_second);
        ImGui::Text("CPU frame time: %.3f ms", telemetry_.cpu_frame_time_ms);
        ImGui::Text("GPU frame time: %.3f ms", telemetry_.gpu_frame_time_ms);

        if (!telemetry_.status_message.empty())
        {
            ImGui::TextWrapped("%s", telemetry_.status_message.c_str());
        }

        for (const auto& series : telemetry_.series)
        {
            if (series.samples.empty())
            {
                continue;
            }

            float minimum = series.minimum;
            float maximum = series.maximum;
            if (std::abs(maximum - minimum) < std::numeric_limits<float>::epsilon())
            {
                maximum = minimum + 1.0F;
            }

            ImGui::PlotLines(series.name.c_str(), series.samples.data(), static_cast<int>(series.samples.size()), 0, nullptr,
                             minimum, maximum, ImVec2(0.0F, 80.0F));
        }
    }

    void ExperimentSandbox::render_runtime_summary()
    {
        ImGui::TextUnformatted("Selection");
        ImGui::Separator();

        if (selected_dataset_index_ < 0 || selected_dataset_index_ >= static_cast<int>(summary_.datasets.size()))
        {
            ImGui::TextDisabled("No dataset selected");
        }
        else
        {
            const auto& dataset = summary_.datasets[static_cast<std::size_t>(selected_dataset_index_)];
            ImGui::Text("Dataset: %s", dataset.identifier.c_str());
            if (!dataset.label.empty())
            {
                ImGui::Text("Label: %s", dataset.label.c_str());
            }
            if (!dataset.kind.empty())
            {
                ImGui::Text("Kind: %s", dataset.kind.c_str());
            }
            if (!dataset.schema_id.empty() || dataset.schema_version > 0)
            {
                if (!dataset.schema_id.empty() && dataset.schema_version > 0)
                {
                    ImGui::Text("Schema: %s (v%d)", dataset.schema_id.c_str(), dataset.schema_version);
                }
                else if (!dataset.schema_id.empty())
                {
                    ImGui::Text("Schema: %s", dataset.schema_id.c_str());
                }
                else
                {
                    ImGui::Text("Schema version: %d", dataset.schema_version);
                }
            }
            if (!dataset.source_generator.empty())
            {
                ImGui::Text("Generator: %s", dataset.source_generator.c_str());
            }
            if (!dataset.tags.empty())
            {
                ImGui::TextUnformatted("Tags:");
                ImGui::Indent();
                for (const auto& tag : dataset.tags)
                {
                    ImGui::BulletText("%s", tag.c_str());
                }
                ImGui::Unindent();
            }
            if (!dataset.statistics.empty())
            {
                std::vector<std::pair<std::string, double>> entries;
                entries.reserve(dataset.statistics.size());
                for (const auto& [name, value] : dataset.statistics)
                {
                    entries.emplace_back(name, value);
                }
                render_numeric_property_block("Statistics", entries);
            }
            for (const auto& group : dataset.statistic_groups)
            {
                if (group.entries.empty())
                {
                    continue;
                }
                std::string heading = "Statistics (" + group.name + ')';
                render_numeric_property_block(heading, group.entries);
            }
            if (!dataset.metrics.empty())
            {
                std::vector<std::pair<std::string, double>> entries;
                entries.reserve(dataset.metrics.size());
                for (const auto& [name, value] : dataset.metrics)
                {
                    entries.emplace_back(name, value);
                }
                render_numeric_property_block("Metrics", entries);
            }
            if (!dataset.source_asset.empty())
            {
                ImGui::Text("Source: %s", dataset.source_asset.c_str());
                if (dataset.source_asset_sha256)
                {
                    ImGui::Text("Source sha256: %s", dataset.source_asset_sha256->c_str());
                }
                if (dataset.source_asset_size_bytes)
                {
                    ImGui::Text("Source size: %s bytes",
                                 format_uintmax(*dataset.source_asset_size_bytes).c_str());
                }
            }
            if (!dataset.processed_asset.empty())
            {
                ImGui::Text("Output: %s", dataset.processed_asset.c_str());
                if (dataset.processed_asset_sha256)
                {
                    ImGui::Text("Output sha256: %s", dataset.processed_asset_sha256->c_str());
                }
                if (dataset.processed_asset_size_bytes)
                {
                    ImGui::Text("Output size: %s bytes",
                                 format_uintmax(*dataset.processed_asset_size_bytes).c_str());
                }
            }
            if (!dataset.remeshing_mode.empty())
            {
                ImGui::Text("Remeshing mode: %s", dataset.remeshing_mode.c_str());
            }
            render_numeric_property_block("Remeshing targets", dataset.remeshing_targets);
            render_property_block("Feature preservation", dataset.feature_preservation);
            render_property_block("Parameterisation", dataset.parameterization_properties);
            if (!dataset.assets.empty())
            {
                ImGui::TextUnformatted("Assets:");
                ImGui::Indent();
                for (const auto& asset : dataset.assets)
                {
                    render_dataset_asset(asset);
                }
                ImGui::Unindent();
            }
        }

        ImGui::Spacing();
        ImGui::TextUnformatted("Runtime");
        ImGui::Separator();

        if (!summary_.runtime.dataset_identifier.empty())
        {
            ImGui::Text("Configured dataset: %s", summary_.runtime.dataset_identifier.c_str());
        }
        if (!summary_.runtime.scene_manifest.empty())
        {
            ImGui::Text("Scene manifest: %s", summary_.runtime.scene_manifest.c_str());
        }
        if (summary_.runtime.scene_manifest_path && !summary_.runtime.scene_manifest_path->empty())
        {
            ImGui::Text("Resolved manifest: %s", summary_.runtime.scene_manifest_path->c_str());
        }
        if (!summary_.runtime.scene_entry_point.empty())
        {
            ImGui::Text("Scene entry: %s", summary_.runtime.scene_entry_point.c_str());
        }
        if (summary_.runtime.schema_version > 0)
        {
            ImGui::Text("Runtime schema version: %d", summary_.runtime.schema_version);
        }
        if (!summary_.runtime.camera_description.empty())
        {
            ImGui::TextWrapped("Camera: %s", summary_.runtime.camera_description.c_str());
        }
        if (!summary_.runtime.simulation_description.empty())
        {
            ImGui::TextWrapped("Simulation: %s", summary_.runtime.simulation_description.c_str());
        }
        ImGui::Text("Hot reload: %s", summary_.runtime.hot_reload.enabled ? "enabled" : "disabled");
        if (summary_.runtime.hot_reload.watch_interval_seconds)
        {
            ImGui::Text("Watch interval: %.3f s", *summary_.runtime.hot_reload.watch_interval_seconds);
        }

        if (!summary_.algorithm_variants.empty())
        {
            ImGui::Spacing();
            ImGui::TextUnformatted("Algorithm Variant");
            ImGui::Separator();

            std::string current_label = "<select>";
            if (selected_algorithm_index_ >= 0
                && selected_algorithm_index_ < static_cast<int>(summary_.algorithm_variants.size()))
            {
                const auto& variant = summary_.algorithm_variants[static_cast<std::size_t>(selected_algorithm_index_)];
                current_label = variant.label.empty() ? variant.identifier : variant.label;
            }

            if (ImGui::BeginCombo("Variant", current_label.c_str()))
            {
                for (std::size_t index = 0; index < summary_.algorithm_variants.size(); ++index)
                {
                    const auto& variant = summary_.algorithm_variants[index];
                    const bool selected = static_cast<int>(index) == selected_algorithm_index_;
                    std::string option_label = variant.label.empty() ? variant.identifier : variant.label;
                    if (ImGui::Selectable(option_label.c_str(), selected))
                    {
                        static_cast<void>(select_algorithm_variant(variant.identifier));
                    }
                    if (selected)
                    {
                        ImGui::SetItemDefaultFocus();
                    }
                }
                ImGui::EndCombo();
            }

            if (selected_algorithm_index_ >= 0
                && selected_algorithm_index_ < static_cast<int>(summary_.algorithm_variants.size()))
            {
                const auto& variant = summary_.algorithm_variants[static_cast<std::size_t>(selected_algorithm_index_)];
                if (variant.description && !variant.description->empty())
                {
                    ImGui::TextWrapped("%s", variant.description->c_str());
                }
            }
        }

        if (summary_.telemetry)
        {
            const auto& telemetry = *summary_.telemetry;
            ImGui::Spacing();
            ImGui::TextUnformatted("Telemetry Configuration");
            ImGui::Separator();
            ImGui::Text("Schema version: %d", telemetry.schema_version);
            if (!telemetry.outputs.empty())
            {
                ImGui::TextUnformatted("Outputs:");
                ImGui::Indent();
                for (const auto& output : telemetry.outputs)
                {
                    std::string target = output.path ? *output.path : std::string{"<unspecified>"};
                    if (output.template_path && !output.template_path->empty())
                    {
                        target += " (template: " + *output.template_path + ')';
                    }
                    ImGui::BulletText("%s → %s", output.kind.c_str(), target.c_str());
                }
                ImGui::Unindent();
            }
            else
            {
                ImGui::TextDisabled("No telemetry outputs configured");
            }

            if (!telemetry.metrics.empty())
            {
                ImGui::TextUnformatted("Metrics:");
                ImGui::Indent();
                for (const auto& metric : telemetry.metrics)
                {
                    ImGui::BulletText("%s (%s)", metric.name.c_str(), metric.statistic.c_str());
                }
                ImGui::Unindent();
            }

            if (telemetry.sampling)
            {
                ImGui::Text("Sampling interval: %d frame(s)", telemetry.sampling->frame_interval);
                ImGui::Text("Include debug overlays: %s",
                             telemetry.sampling->include_debug_overlays ? "yes" : "no");
            }
        }
    }

    bool ExperimentSandbox::matches_dataset_filter(std::string_view text) const
    {
        if (dataset_filter_lower_.empty())
        {
            return true;
        }
        auto lowered = to_lower(text);
        return lowered.find(dataset_filter_lower_) != std::string::npos;
    }

    bool ExperimentSandbox::sync_overlay_preferences()
    {
        if (selected_preset_index_ < 0 || selected_preset_index_ >= static_cast<int>(summary_.rendering_presets.size()))
        {
            const bool changed = !preferences_.overlays.empty();
            preferences_.overlays.clear();
            return changed;
        }

        const auto& overlays = summary_.rendering_presets[static_cast<std::size_t>(selected_preset_index_)].overlays;
        std::unordered_map<std::string, bool> synchronised;
        synchronised.reserve(overlays.size());
        for (const auto& overlay : overlays)
        {
            auto it = preferences_.overlays.find(overlay.key);
            const bool enabled = (it != preferences_.overlays.end()) ? it->second : overlay.default_enabled;
            synchronised.emplace(overlay.key, enabled);
        }
        const bool changed = preferences_.overlays != synchronised;
        preferences_.overlays = std::move(synchronised);
        return changed;
    }

    void ExperimentSandbox::notify_preference_changes(const SandboxPreferences& previous)
    {
        if (callbacks_.on_dataset_selected && preferences_.selected_dataset != previous.selected_dataset)
        {
            callbacks_.on_dataset_selected(preferences_.selected_dataset);
        }

        if (callbacks_.on_algorithm_selected
            && preferences_.selected_algorithm_variant != previous.selected_algorithm_variant)
        {
            callbacks_.on_algorithm_selected(preferences_.selected_algorithm_variant);
        }

        if (callbacks_.on_case_study_selected
            && preferences_.selected_case_study != previous.selected_case_study)
        {
            callbacks_.on_case_study_selected(preferences_.selected_case_study);
        }

        if (callbacks_.on_rendering_changed)
        {
            const bool preset_changed = preferences_.selected_preset != previous.selected_preset;
            const bool shading_changed = preferences_.shading_mode != previous.shading_mode;
            const bool overlays_changed = preferences_.overlays != previous.overlays;
            const bool resolution_changed = preferences_.resolution_width != previous.resolution_width
                || preferences_.resolution_height != previous.resolution_height;
            if (preset_changed || shading_changed || overlays_changed || resolution_changed)
            {
                callbacks_.on_rendering_changed(preferences_);
            }
        }
    }

    const BenchmarkScenarioDescriptor* ExperimentSandbox::find_active_benchmark_scenario() const
    {
        if (!summary_.benchmarks)
        {
            return nullptr;
        }

        const auto& scenarios = summary_.benchmarks->scenarios;
        if (scenarios.empty())
        {
            return nullptr;
        }

        const auto matches = [&](const BenchmarkScenarioDescriptor& descriptor) {
            if (!descriptor.dataset.empty() && descriptor.dataset != preferences_.selected_dataset)
            {
                return false;
            }
            if (!descriptor.runtime_profile.empty()
                && descriptor.runtime_profile != preferences_.selected_algorithm_variant)
            {
                return false;
            }
            if (!descriptor.rendering_preset.empty() && descriptor.rendering_preset != preferences_.selected_preset)
            {
                return false;
            }
            return true;
        };

        for (const auto& scenario : scenarios)
        {
            if (matches(scenario))
            {
                return &scenario;
            }
        }

        return &scenarios.front();
    }

    void ExperimentSandbox::apply_benchmark_result(SandboxBenchmarkResult result)
    {
        last_benchmark_result_ = std::move(result);
    }

    const std::optional<SandboxBenchmarkResult>& ExperimentSandbox::last_benchmark_result() const noexcept
    {
        return last_benchmark_result_;
    }
} // namespace engine::tools::sandbox

