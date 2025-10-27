#include "remesh_cli.hpp"

#include <cstddef>
#include <iostream>
#include <span>

int main(int argc, char** argv)
{
    using engine::geometry::tools::ExecuteRemesh;
    using engine::geometry::tools::ParseArguments;
    using engine::geometry::tools::PrintHelp;
    using engine::geometry::tools::PrintSummary;

    const std::span arguments(argv, static_cast<std::size_t>(argc));
    const auto options_result = ParseArguments(arguments);
    if (!options_result.has_value())
    {
        std::cerr << options_result.error() << "\n\n";
        PrintHelp(std::cerr);
        return 1;
    }

    const engine::geometry::tools::RemeshCliOptions options = options_result.value();
    if (options.show_help)
    {
        PrintHelp(std::cout);
        return 0;
    }

    const auto execution = ExecuteRemesh(options);
    if (!execution.has_value())
    {
        std::cerr << execution.error() << '\n';
        return 2;
    }

    PrintSummary(options, execution.value(), std::cout);
    return 0;
}