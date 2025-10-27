#pragma once

#include <chrono>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "engine/tools/api.hpp"

namespace engine::tools::profiling
{
    struct ProfileEntry
    {
        std::string name;
        double duration_ms{0.0};
        std::uint64_t call_count{0};
        double min_ms{0.0};
        double max_ms{0.0};
        double average_ms{0.0};
    };

    struct ProfileReport
    {
        std::vector<ProfileEntry> entries;
        double total_duration_ms{0.0};
    };

    class ENGINE_TOOLS_API Profiler
    {
    public:
        Profiler() = default;
        ~Profiler() = default;

        void begin(std::string_view name);
        void end(std::string_view name);

        void reset();
        [[nodiscard]] ProfileReport generate_report() const;

    private:
        struct TimingData
        {
            std::chrono::steady_clock::time_point start_time;
            double total_ms{0.0};
            double min_ms{std::numeric_limits<double>::max()};
            double max_ms{0.0};
            std::uint64_t call_count{0};
            bool is_active{false};
        };

        std::unordered_map<std::string, TimingData> timings_;
    };

    class ENGINE_TOOLS_API ScopedProfile
    {
    public:
        ScopedProfile(Profiler& profiler, std::string_view name);
        ~ScopedProfile();

        ScopedProfile(const ScopedProfile&) = delete;
        ScopedProfile& operator=(const ScopedProfile&) = delete;

    private:
        Profiler& profiler_;
        std::string name_;
    };

    // Global profiler instance
    ENGINE_TOOLS_API Profiler& global_profiler();
} // namespace engine::tools::profiling

// Convenience macro for scoped profiling
#define PROFILE_SCOPE(name) \
engine::tools::profiling::ScopedProfile profile_scope_##__LINE__( \
engine::tools::profiling::global_profiler(), name)