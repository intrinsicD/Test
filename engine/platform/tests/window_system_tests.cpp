#include "engine/platform/windowing/window.hpp"

#include <cstdlib>
#include <optional>
#include <string>
#include <string_view>

#include <gtest/gtest.h>

namespace {

class ScopedEnvVar {
public:
    ScopedEnvVar(const char* name, const char* value)
        : name_(name) {
        if (const char* current = std::getenv(name_); current != nullptr) {
            previous_.emplace(current);
        }
        set(value);
    }

    ScopedEnvVar(const ScopedEnvVar&) = delete;
    ScopedEnvVar& operator=(const ScopedEnvVar&) = delete;

    ScopedEnvVar(ScopedEnvVar&&) = delete;
    ScopedEnvVar& operator=(ScopedEnvVar&&) = delete;

    ~ScopedEnvVar() {
        if (previous_) {
            set(previous_->c_str());
        } else {
            set(nullptr);
        }
    }

private:
    void set(const char* value) {
#if defined(_WIN32)
        if (value != nullptr) {
            _putenv_s(name_, value);
        } else {
            _putenv_s(name_, "");
        }
#else
        if (value != nullptr) {
            ::setenv(name_, value, 1);
        } else {
            ::unsetenv(name_);
        }
#endif
    }

    const char* name_;
    std::optional<std::string> previous_{};
};

class ScopedBackendOverride : public ScopedEnvVar {
public:
    explicit ScopedBackendOverride(const char* value)
        : ScopedEnvVar("ENGINE_PLATFORM_WINDOW_BACKEND", value) {}
};

}  // namespace

TEST(WindowSystem, ExplicitSdlBackendCreatesWindow) {
    using namespace engine::platform;

    WindowConfig config;
    auto window = create_window(config, WindowBackend::SDL);
    ASSERT_NE(window, nullptr);
    EXPECT_EQ(window->backend_name(), "sdl");
}

TEST(WindowSystem, CapabilityRequirementsRejectMockBackend) {
    using namespace engine::platform;

    WindowConfig config;
    config.capability_requirements.require_native_surface = true;

    EXPECT_THROW(create_window(config, WindowBackend::Mock), std::runtime_error);
}

TEST(WindowSystem, AutoSelectionSkipsBackendsWithoutRequiredCapabilities) {
    using namespace engine::platform;

    ScopedBackendOverride clear_override{nullptr};

    WindowConfig config;
    config.capability_requirements.require_native_surface = true;

    auto window = create_window(config, WindowBackend::Auto);
    ASSERT_NE(window, nullptr);
    EXPECT_NE(window->backend_name(), std::string_view{"mock"});
}

TEST(WindowSystem, EnvironmentOverrideSelectsRequestedBackend) {
    using namespace engine::platform;

    ScopedBackendOverride override_backend{"mock"};

    WindowConfig config;
    auto window = create_window(config, WindowBackend::Auto);
    ASSERT_NE(window, nullptr);
    EXPECT_EQ(window->backend_name(), "mock");
}
