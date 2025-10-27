#include "engine/scene/components/hierarchy.hpp"
#include "engine/scene/scene.hpp"
#include "engine/scene/serialization/serializer.hpp"
#include "engine/scene/validation.hpp"
#include "engine/scene/components/transform.hpp"

#include <cctype>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <limits>
#include <type_traits>
#include <utility>
#include <vector>

namespace scene = engine::scene;
namespace validation = engine::scene::validation;

namespace
{
    constexpr std::string_view kValidHierarchyScene = R"(scene "ValidHierarchy" 3
entity 0 4
component Name "Root"
component Hierarchy 4294967295 1 4294967295 4294967295
component LocalTransform 1 1 1 1 0 0 0 0 0 0
component WorldTransform 1 1 1 1 0 0 0 0 0 0
entity_end
entity 1 4
component Name "Child"
component Hierarchy 0 2 4294967295 4294967295
component LocalTransform 1 1 1 1 0 0 0 0 0 0
component WorldTransform 1 1 1 1 0 0 0 0 0 0
entity_end
entity 2 4
component Name "Grandchild"
component Hierarchy 1 4294967295 4294967295 4294967295
component LocalTransform 1 1 1 1 0 0 0 0 0 0
component WorldTransform 1 1 1 1 0 0 0 0 0 0
entity_end
scene_end
)";

    constexpr std::string_view kInvalidHierarchyScene = R"(scene "InvalidHierarchy" 5
entity 0 4
component Name "Root"
component Hierarchy 4294967295 1 4294967295 4294967295
component LocalTransform 1 1 1 1 0 0 0 0 0 0
component WorldTransform 1 1 1 1 0 0 0 0 0 0
entity_end
entity 1 4
component Name "CycleChild"
component Hierarchy 0 2 4294967295 4294967295
component LocalTransform 1 1 1 1 0 0 0 0 0 0
component WorldTransform 1 1 1 1 0 0 0 0 0 0
entity_end
entity 2 4
component Name "DanglingChild"
component Hierarchy 0 3 4294967295 1
component LocalTransform 1 1 1 1 0 0 0 0 0 0
component WorldTransform 1 1 1 1 0 0 0 0 0 0
entity_end
entity 3 4
component Name "MissingHierarchyParent"
component Hierarchy 0 4 4294967295 2
component LocalTransform 1 1 1 1 0 0 0 0 0 0
component WorldTransform 1 1 1 1 0 0 0 0 0 0
entity_end
entity 4 4
component Name "OrphanChild"
component Hierarchy 3 4294967295 4294967295 2
component LocalTransform 1 1 1 1 0 0 0 0 0 0
component WorldTransform 1 1 1 1 0 0 0 0 0 0
entity_end
scene_end
)";

    struct SampleDefinition
    {
        std::string name{};
        std::string description{};
        std::string_view scene_data{};
        void (*mutate)(scene::Scene&) = nullptr;
    };

    void mutate_invalid_scene(scene::Scene& scene_instance)
    {
        auto& registry = scene_instance.registry();
        const entt::entity root = entt::entity{0};
        const entt::entity cycle_child = entt::entity{1};
        const entt::entity dangling_child = entt::entity{2};
        const entt::entity missing_parent = entt::entity{3};
        const entt::entity orphan_child = entt::entity{4};

        auto& root_hierarchy = registry.get<scene::components::Hierarchy>(root);
        root_hierarchy.parent = cycle_child;

        auto& cycle_hierarchy = registry.get<scene::components::Hierarchy>(cycle_child);
        cycle_hierarchy.next_sibling = dangling_child;

        auto& dangling_hierarchy = registry.get<scene::components::Hierarchy>(dangling_child);
        dangling_hierarchy.parent = static_cast<scene::Scene::entity_type>(123456789);
        dangling_hierarchy.previous_sibling = cycle_child;
        dangling_hierarchy.next_sibling = orphan_child;

        auto& dangling_local = registry.get<scene::components::LocalTransform>(dangling_child);
        dangling_local.value.translation[0] = 2.0F;
        dangling_local.value.translation[1] = -1.0F;
        dangling_local.value.translation[2] = 0.5F;

        auto& dangling_world = registry.get<scene::components::WorldTransform>(dangling_child);
        dangling_world.value.translation[0] = 8.0F;
        dangling_world.value.translation[1] = -4.0F;
        dangling_world.value.translation[2] = 1.0F;

        auto& orphan_hierarchy = registry.get<scene::components::Hierarchy>(orphan_child);
        orphan_hierarchy.parent = missing_parent;
        orphan_hierarchy.previous_sibling = dangling_child;

        registry.remove<scene::components::Hierarchy>(missing_parent);
    }

    [[nodiscard]] const std::map<std::string, SampleDefinition>& sample_definitions()
    {
        static const std::map<std::string, SampleDefinition> definitions = {
            {
                "valid_hierarchy",
                SampleDefinition{
                    "valid_hierarchy",
                    "Three-node hierarchy with consistent transforms and relationships.",
                    kValidHierarchyScene,
                    nullptr,
                },
            },
            {
                "invalid_hierarchy",
                SampleDefinition{
                    "invalid_hierarchy",
                    "Scene exercising cycles, dangling parents, missing hierarchy components, and inconsistent transforms.",
                    kInvalidHierarchyScene,
                    mutate_invalid_scene,
                },
            },
        };
        return definitions;
    }

    struct CommandLineOptions
    {
        bool show_help{false};
        bool list_samples{false};
        bool pretty_json{false};
        bool fail_on_issues{false};
        std::optional<std::filesystem::path> emit_samples_dir{};
        std::vector<std::filesystem::path> scene_files{};
        std::vector<std::string> sample_names{};
    };

    [[nodiscard]] std::string escape_json(std::string_view text)
    {
        std::ostringstream stream;
        stream << std::hex << std::setfill('0');

        for (unsigned char ch : text)
        {
            switch (ch)
            {
            case '\\':
                stream << "\\\\";
                break;
            case '"':
                stream << "\\\"";
                break;
            case '\b':
                stream << "\\b";
                break;
            case '\f':
                stream << "\\f";
                break;
            case '\n':
                stream << "\\n";
                break;
            case '\r':
                stream << "\\r";
                break;
            case '\t':
                stream << "\\t";
                break;
            default:
                if (std::iscntrl(ch))
                {
                    stream << "\\u" << std::setw(4) << static_cast<int>(ch);
                }
                else
                {
                    stream << static_cast<char>(ch);
                }
                break;
            }
        }

        return stream.str();
    }

    [[nodiscard]] std::string entity_to_string(entt::entity entity)
    {
        using underlying_type = std::underlying_type_t<entt::entity>;
        if (entity == entt::null)
        {
            return "null";
        }

        return std::to_string(static_cast<underlying_type>(entity));
    }

    void print_json_report(const validation::HierarchyValidationReport& report,
                           std::ostream& output,
                           bool pretty)
    {
        const auto indent = [&](int level)
        {
            if (pretty)
            {
                output << std::string(level * 2, ' ');
            }
        };

        output << '{';
        if (pretty)
        {
            output << '\n';
        }
        indent(1);
        output << "\"ok\": " << (report.ok() ? "true" : "false") << ',';
        if (pretty)
        {
            output << '\n';
        }
        indent(1);
        output << "\"metrics\": {";
        if (pretty)
        {
            output << '\n';
            indent(2);
        }
        else
        {
            output << ' ';
        }
        output << "\"issue_count\": " << report.metrics.issue_count << ',';
        if (pretty)
        {
            output << '\n';
            indent(2);
        }
        else
        {
            output << ' ';
        }
        output << "\"cycle_count\": " << report.metrics.cycle_count << ',';
        if (pretty)
        {
            output << '\n';
            indent(2);
        }
        else
        {
            output << ' ';
        }
        output << "\"dangling_parent_count\": " << report.metrics.dangling_parent_count << ',';
        if (pretty)
        {
            output << '\n';
            indent(2);
        }
        else
        {
            output << ' ';
        }
        output << "\"missing_parent_hierarchy_count\": " << report.metrics.missing_parent_hierarchy_count << ',';
        if (pretty)
        {
            output << '\n';
            indent(2);
        }
        else
        {
            output << ' ';
        }
        output << "\"non_finite_transform_count\": " << report.metrics.non_finite_transform_count << ',';
        if (pretty)
        {
            output << '\n';
            indent(2);
        }
        else
        {
            output << ' ';
        }
        output << "\"transform_mismatch_count\": " << report.metrics.transform_mismatch_count;
        if (pretty)
        {
            output << '\n';
            indent(1);
            output << "},\n";
        }
        else
        {
            output << " },";
        }

        indent(1);
        output << "\"issues\": [";
        if (report.issues.empty())
        {
            output << ']';
            if (pretty)
            {
                output << '\n';
                indent(0);
            }
            output << '}';
            return;
        }

        if (pretty)
        {
            output << '\n';
        }
        else
        {
            output << ' ';
        }

        for (std::size_t index = 0; index < report.issues.size(); ++index)
        {
            const auto& issue = report.issues[index];
            if (pretty)
            {
                indent(2);
            }
            output << '{';
            if (pretty)
            {
                output << '\n';
                indent(3);
            }
            else
            {
                output << ' ';
            }
            output << "\"entity\": " << entity_to_string(issue.entity) << ',';
            if (pretty)
            {
                output << '\n';
                indent(3);
            }
            else
            {
                output << ' ';
            }
            output << "\"related\": " << entity_to_string(issue.related) << ',';
            if (pretty)
            {
                output << '\n';
                indent(3);
            }
            else
            {
                output << ' ';
            }
            output << "\"type\": \"" << escape_json(validation::to_string(issue.type)) << "\",";
            if (pretty)
            {
                output << '\n';
                indent(3);
            }
            else
            {
                output << ' ';
            }
            output << "\"message\": \"" << escape_json(issue.message) << "\"";
            if (pretty)
            {
                output << '\n';
                indent(2);
                output << '}';
                if (index + 1U < report.issues.size())
                {
                    output << ',';
                }
                output << '\n';
            }
            else
            {
                output << " }";
                if (index + 1U < report.issues.size())
                {
                    output << ", ";
                }
                else
                {
                    output << ' ';
                }
            }
        }

        if (pretty)
        {
            indent(1);
            output << ']';
            output << '\n';
            indent(0);
            output << '}';
        }
        else
        {
            output << ']';
            output << '}';
        }
    }

    void print_summary(const std::string& label, const validation::HierarchyValidationReport& report)
    {
        std::cout << "Scene: " << label << '\n';
        std::cout << "  Status: " << (report.ok() ? "clean" : "issues detected") << '\n';
        std::cout << "  Issue count: " << report.metrics.issue_count << '\n';

        for (const auto& issue : report.issues)
        {
            std::cout << "    - [" << validation::to_string(issue.type) << "] entity="
                << entity_to_string(issue.entity);
            if (issue.related != entt::null)
            {
                std::cout << " related=" << entity_to_string(issue.related);
            }
            std::cout << " message=\"" << issue.message << "\"\n";
        }
    }

    [[nodiscard]] CommandLineOptions parse_command_line(int argc, char* argv[])
    {
        CommandLineOptions options{};
        for (int index = 1; index < argc; ++index)
        {
            std::string_view argument{argv[index]};
            if (argument == "--help" || argument == "-h")
            {
                options.show_help = true;
            }
            else if (argument == "--list-samples")
            {
                options.list_samples = true;
            }
            else if (argument == "--pretty")
            {
                options.pretty_json = true;
            }
            else if (argument == "--fail-on-issues")
            {
                options.fail_on_issues = true;
            }
            else if (argument == "--scene")
            {
                if (index + 1 >= argc)
                {
                    throw std::runtime_error{"Missing value after --scene"};
                }
                options.scene_files.emplace_back(argv[++index]);
            }
            else if (argument == "--sample")
            {
                if (index + 1 >= argc)
                {
                    throw std::runtime_error{"Missing value after --sample"};
                }
                options.sample_names.emplace_back(argv[++index]);
            }
            else if (argument == "--emit-samples")
            {
                if (index + 1 >= argc)
                {
                    throw std::runtime_error{"Missing value after --emit-samples"};
                }
                options.emit_samples_dir = std::filesystem::path{argv[++index]};
            }
            else
            {
                std::ostringstream message;
                message << "Unrecognised argument: " << argument;
                throw std::runtime_error{message.str()};
            }
        }

        return options;
    }

    void print_usage(std::ostream& output)
    {
        output << "Usage: scene_hierarchy_diagnostics_sample [options]\n\n";
        output << "Options:\n";
        output << "  --scene <path>          Validate the specified .scene file (repeatable).\n";
        output << "  --sample <name>         Validate a built-in sample (repeatable).\n";
        output << "  --list-samples          List available built-in samples.\n";
        output << "  --emit-samples <dir>    Write built-in samples to the target directory.\n";
        output << "  --fail-on-issues        Return a non-zero exit code when issues are detected.\n";
        output << "  --pretty                Pretty-print JSON output.\n";
        output << "  --help                  Display this message.\n";
    }

    void ensure_directory(const std::filesystem::path& directory)
    {
        std::error_code ec{};
        if (!std::filesystem::exists(directory, ec))
        {
            if (!std::filesystem::create_directories(directory, ec))
            {
                std::ostringstream message;
                message << "Failed to create directory '" << directory.string() << "': " << ec.message();
                throw std::runtime_error{message.str()};
            }
        }
    }

    void emit_samples_to_directory(const std::filesystem::path& directory)
    {
        ensure_directory(directory);
        for (const auto& [name, definition] : sample_definitions())
        {
            const auto target_path = directory / (name + ".scene");
            std::ofstream output{target_path};
            if (!output)
            {
                std::ostringstream message;
                message << "Failed to open '" << target_path.string() << "' for writing";
                throw std::runtime_error{message.str()};
            }

            if (definition.mutate == nullptr)
            {
                output << definition.scene_data;
            }
            else
            {
                scene::Scene scene_instance{definition.name};
                std::istringstream input{std::string(definition.scene_data)};
                scene::serialization::load(scene_instance, input);
                definition.mutate(scene_instance);
                scene::serialization::save(scene_instance, output);
            }
            std::cout << "Wrote sample: " << target_path << '\n';
        }
    }

    [[nodiscard]] validation::HierarchyValidationReport validate_scene(scene::Scene& scene_instance)
    {
        return validation::validate_hierarchy(scene_instance);
    }

    [[nodiscard]] validation::HierarchyValidationReport validate_file(const std::filesystem::path& path,
                                                                      scene::Scene& scene_instance)
    {
        std::ifstream input{path};
        if (!input)
        {
            std::ostringstream message;
            message << "Failed to open scene file '" << path.string() << "'";
            throw std::runtime_error{message.str()};
        }

        scene::serialization::load(scene_instance, input);
        return validate_scene(scene_instance);
    }

    void list_samples()
    {
        std::cout << "Available samples:\n";
        for (const auto& [name, definition] : sample_definitions())
        {
            std::cout << "  - " << name;
            if (!definition.description.empty())
            {
                std::cout << ": " << definition.description;
            }
            std::cout << '\n';
        }
    }

    validation::HierarchyValidationReport validate_sample_definition(const SampleDefinition& definition)
    {
        scene::Scene scene_instance{definition.name};
        std::istringstream input{std::string(definition.scene_data)};
        scene::serialization::load(scene_instance, input);
        if (definition.mutate != nullptr)
        {
            definition.mutate(scene_instance);
        }
        return validate_scene(scene_instance);
    }
} // namespace

int main(int argc, char* argv[])
{
    try
    {
        const auto options = parse_command_line(argc, argv);
        if (options.show_help)
        {
            print_usage(std::cout);
            return 0;
        }

        if (options.list_samples)
        {
            list_samples();
            return 0;
        }

        if (options.emit_samples_dir.has_value())
        {
            emit_samples_to_directory(options.emit_samples_dir.value());
        }

        const bool has_inputs = !options.scene_files.empty() || !options.sample_names.empty();
        if (!has_inputs)
        {
            std::cout << "No scenes provided. Use --scene or --sample, or --help for usage information." << '\n';
            return 0;
        }

        bool encountered_issues = false;
        for (const auto& scene_path : options.scene_files)
        {
            scene::Scene scene_instance{"FileScene"};
            const auto report = validate_file(scene_path, scene_instance);
            print_summary(scene_path.string(), report);
            print_json_report(report, std::cout, options.pretty_json);
            std::cout << '\n';
            encountered_issues = encountered_issues || !report.ok();
        }

        const auto samples = sample_definitions();
        for (const auto& name : options.sample_names)
        {
            const auto it = samples.find(name);
            if (it == samples.end())
            {
                std::cerr << "Unknown sample: " << name << '\n';
                return 1;
            }

            const auto report = validate_sample_definition(it->second);
            print_summary(name, report);
            print_json_report(report, std::cout, options.pretty_json);
            std::cout << '\n';
            encountered_issues = encountered_issues || !report.ok();
        }

        if (options.fail_on_issues && encountered_issues)
        {
            return 1;
        }

        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << "Error: " << error.what() << '\n';
        print_usage(std::cerr);
        return 1;
    }
}