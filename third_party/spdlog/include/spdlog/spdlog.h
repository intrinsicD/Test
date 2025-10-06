#pragma once

#include <chrono>
#include <format>
#include <iostream>
#include <mutex>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <utility>

namespace spdlog {

namespace detail {
inline std::mutex& global_mutex() {
    static std::mutex mutex;
    return mutex;
}

inline void write_line(std::string_view level, std::string_view message) {
    const auto now = std::chrono::system_clock::now();
    const auto time = std::chrono::system_clock::to_time_t(now);
    std::lock_guard<std::mutex> lock(global_mutex());
    std::cout << "[" << level << "] " << std::string(message) << std::endl;
    (void)time;
}

inline void log_message(std::string_view level, std::string_view message) {
    write_line(level, message);
}

template <typename... Args>
inline void log_message(std::string_view level, std::string_view fmt, Args&&... args) {
    if constexpr (sizeof...(Args) == 0) {
        log_message(level, fmt);
    } else {
        auto arguments = std::tuple<std::decay_t<Args>...>(std::forward<Args>(args)...);
        const auto formatted = std::apply(
            [&](auto&... stored) {
                return std::vformat(fmt, std::make_format_args(stored...));
            },
            arguments);
        log_message(level, formatted);
    }
}
}  // namespace detail

inline void set_level(int) {}

inline void info(std::string_view message) {
    detail::log_message("info", message);
}

template <typename... Args>
inline void info(std::string_view fmt, Args&&... args) {
    detail::log_message("info", fmt, std::forward<Args>(args)...);
}

inline void warn(std::string_view message) {
    detail::log_message("warn", message);
}

template <typename... Args>
inline void warn(std::string_view fmt, Args&&... args) {
    detail::log_message("warn", fmt, std::forward<Args>(args)...);
}

inline void error(std::string_view message) {
    detail::log_message("error", message);
}

template <typename... Args>
inline void error(std::string_view fmt, Args&&... args) {
    detail::log_message("error", fmt, std::forward<Args>(args)...);
}

inline void debug(std::string_view message) {
    detail::log_message("debug", message);
}

template <typename... Args>
inline void debug(std::string_view fmt, Args&&... args) {
    detail::log_message("debug", fmt, std::forward<Args>(args)...);
}

inline void trace(std::string_view message) {
    detail::log_message("trace", message);
}

template <typename... Args>
inline void trace(std::string_view fmt, Args&&... args) {
    detail::log_message("trace", fmt, std::forward<Args>(args)...);
}

}  // namespace spdlog

