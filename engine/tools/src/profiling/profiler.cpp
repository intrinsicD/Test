#include "engine/tools/profiling/profiler.hpp"

#include <algorithm>
#include <limits>
#include <string_view>

namespace engine::tools::profiling
{
    void Profiler::begin(std::string_view name)
    {
        auto& timing = timings_[std::string(name)];
        timing.start_time = std::chrono::steady_clock::now();
        timing.is_active = true;
    }

    void Profiler::end(std::string_view name)
    {
        auto end_time = std::chrono::steady_clock::now();

        auto it = timings_.find(std::string(name));
        if (it == timings_.end() || !it->second.is_active)
        {
            return; // No matching begin() call
        }

        auto& timing = it->second;
        auto duration = static_cast<double>(std::chrono::duration_cast<std::chrono::microseconds>(
            end_time - timing.start_time).count()) / 1000.0;

        timing.total_ms += duration;
        timing.min_ms = std::min(timing.min_ms, duration);
        timing.max_ms = std::max(timing.max_ms, duration);
        timing.call_count++;
        timing.is_active = false;
    }

    void Profiler::reset()
    {
        timings_.clear();
    }

    ProfileReport Profiler::generate_report() const
    {
        ProfileReport report;

        for (const auto& [name, timing] : timings_)
        {
            if (timing.call_count == 0)
            {
                continue;
            }

            ProfileEntry entry;
            entry.name = name;
            entry.duration_ms = timing.total_ms;
            entry.call_count = timing.call_count;
            entry.min_ms = timing.min_ms;
            entry.max_ms = timing.max_ms;
            entry.average_ms = timing.total_ms / static_cast<double>(timing.call_count);

            report.entries.push_back(entry);
            report.total_duration_ms += timing.total_ms;
        }

        // Sort by total duration (descending)
        std::sort(report.entries.begin(), report.entries.end(),
                  [](const ProfileEntry& a, const ProfileEntry& b)
                  {
                      return a.duration_ms > b.duration_ms;
                  });

        return report;
    }

    ScopedProfile::ScopedProfile(Profiler& profiler, std::string_view name)
        : profiler_(profiler), name_(name)
    {
        profiler_.begin(name_);
    }

    ScopedProfile::~ScopedProfile()
    {
        profiler_.end(name_);
    }

    Profiler& global_profiler()
    {
        static Profiler instance;
        return instance;
    }
} // namespace engine::tools::profiling