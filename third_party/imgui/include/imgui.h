#pragma once

#include <cstdarg>
#include <cstdio>
#include <string>
#include <string_view>

namespace ImGui {

inline bool Begin(const char* name, bool* = nullptr, int = 0) {
    std::printf("[ImGui] Begin: %s\n", name ? name : "");
    return true;
}

inline void End() {
    std::printf("[ImGui] End\n");
}

inline void Text(const char* fmt, ...) {
    std::va_list args;
    va_start(args, fmt);
    std::vprintf(fmt, args);
    std::printf("\n");
    va_end(args);
}

inline void Separator() {
    std::printf("[ImGui] ------------------------------\n");
}

inline void Value(const char* prefix, std::string_view value) {
    std::printf("%s%.*s\n", prefix ? prefix : "", static_cast<int>(value.size()), value.data());
}

inline void Value(const char* prefix, int value) {
    std::printf("%s%d\n", prefix ? prefix : "", value);
}

inline void Value(const char* prefix, std::size_t value) {
    std::printf("%s%zu\n", prefix ? prefix : "", value);
}

}  // namespace ImGui

