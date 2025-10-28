#include "engine/tools/sandbox/experiment_sandbox.hpp"

#include <imgui.h>

#include <algorithm>
#include <cctype>
#include <charconv>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <limits>
#include <sstream>
#include <system_error>

namespace engine::tools::sandbox
{
    namespace
    {
        std::string to_lower(std::string_view input)
        {
            std::string output{input};
            std::transform(output.begin(), output.end(), output.begin(), [](unsigned char ch) {
                return static_cast<char>(std::tolower(ch));
            });
            return output;
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
        last_benchmark_result_.reset();

        for (std::size_t index = 0; index < summary_.datasets.size(); ++index)
        {
            dataset_lookup_.emplace(summary_.datasets[index].identifier, index);
        }

        for (std::size_t index = 0; index < summary_.rendering_presets.size(); ++index)
        {
            preset_lookup_.emplace(summary_.rendering_presets[index].identifier, index);
        }

        if (summary_.selected_dataset)
        {
            preferences_.selected_dataset = *summary_.selected_dataset;
        }

        ensure_selection_defaults();
        notify_preference_changes(previous_preferences);
    }

    void ExperimentSandbox::update_telemetry(const TelemetrySnapshot& telemetry)
    {
        telemetry_ = telemetry;
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

    void ExperimentSandbox::set_preferences(const SandboxPreferences& preferences)
    {
        const SandboxPreferences previous_preferences = preferences_;
        preferences_ = preferences;
        ensure_selection_defaults();
        notify_preference_changes(previous_preferences);
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
        stream << "shading_mode=" << preferences_.shading_mode << '\n';
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
            if (callbacks_.on_run_benchmark)
            {
                apply_benchmark_result(callbacks_.on_run_benchmark(preferences_));
            }
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
                ImGui::TextUnformatted("Statistics:");
                ImGui::Indent();
                for (const auto& [name, value] : dataset.statistics)
                {
                    ImGui::BulletText("%s: %.3f", name.c_str(), value);
                }
                ImGui::Unindent();
            }
            if (!dataset.metrics.empty())
            {
                ImGui::TextUnformatted("Metrics:");
                ImGui::Indent();
                for (const auto& [name, value] : dataset.metrics)
                {
                    ImGui::BulletText("%s: %.3f", name.c_str(), value);
                }
                ImGui::Unindent();
            }
            if (!dataset.source_asset.empty())
            {
                ImGui::Text("Source: %s", dataset.source_asset.c_str());
            }
            if (!dataset.processed_asset.empty())
            {
                ImGui::Text("Output: %s", dataset.processed_asset.c_str());
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
        if (!summary_.runtime.scene_entry_point.empty())
        {
            ImGui::Text("Scene entry: %s", summary_.runtime.scene_entry_point.c_str());
        }
        if (!summary_.runtime.camera_description.empty())
        {
            ImGui::TextWrapped("Camera: %s", summary_.runtime.camera_description.c_str());
        }
        if (!summary_.runtime.simulation_description.empty())
        {
            ImGui::TextWrapped("Simulation: %s", summary_.runtime.simulation_description.c_str());
        }
        ImGui::Text("Hot reload: %s", summary_.runtime.hot_reload_enabled ? "enabled" : "disabled");
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

        if (callbacks_.on_rendering_changed)
        {
            const bool preset_changed = preferences_.selected_preset != previous.selected_preset;
            const bool shading_changed = preferences_.shading_mode != previous.shading_mode;
            const bool overlays_changed = preferences_.overlays != previous.overlays;
            if (preset_changed || shading_changed || overlays_changed)
            {
                callbacks_.on_rendering_changed(preferences_);
            }
        }
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

