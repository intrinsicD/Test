#pragma once

#include "engine/platform/api.hpp"

#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <variant>

namespace engine::platform {

/// \brief Describes the windowing backend that should service a window instance.
enum class WindowBackend {
    /// Selects the most appropriate backend for the current platform using the
    /// build-time default (`ENGINE_WINDOW_BACKEND`) and runtime overrides, while
    /// falling back to other supported backends when the preferred choice is
    /// unavailable or incompatible with the requested capabilities.
    Auto,
    /// Stub entry for a GLFW-driven implementation.
    GLFW,
    /// Stub entry for a SDL-driven implementation.
    SDL,
    /// Headless mock implementation used for tests and server environments.
    Mock,
};

/// \brief Describes the capabilities provided by a window backend.
struct WindowBackendCapabilities {
    /// True when the backend can operate without an active display connection.
    bool headless_safe{false};
    /// True when the backend provides a native surface handle suitable for
    /// swapchain creation.
    bool native_surface{false};
};

/// \brief Human readable configuration for constructing a window.
struct WindowConfig {
    /// \brief Capability requirements that may constrain backend selection.
    struct CapabilityRequirements {
        /// Require the backend to run without a display connection. When true
        /// only backends that advertise headless support are eligible.
        bool require_headless_safe{false};
        /// Require the backend to expose a native surface handle that can be
        /// consumed by a renderer swapchain implementation.
        bool require_native_surface{false};
    };

    /// Desired window title. Implementations copy this string during
    /// construction.
    std::string title{"Engine"};
    /// Initial pixel width of the window client area.
    std::uint32_t width{1280};
    /// Initial pixel height of the window client area.
    std::uint32_t height{720};
    /// Whether the window should be initially visible.
    bool visible{true};
    /// Whether the window may be resized by the user.
    bool resizable{true};
    /// Capability requirements constraining backend selection.
    CapabilityRequirements capability_requirements{};

    /// Returns true when the configuration requires a headless-safe backend.
    [[nodiscard]] bool requires_headless_safe() const noexcept {
        return capability_requirements.require_headless_safe;
    }

    /// Returns true when the configuration requires a native swapchain surface.
    [[nodiscard]] bool requires_native_surface() const noexcept {
        return capability_requirements.require_native_surface;
    }

    /// Evaluates whether the supplied backend capabilities satisfy the
    /// configuration requirements.
    [[nodiscard]] bool allows_backend(const WindowBackendCapabilities& capabilities) const noexcept {
        if (requires_headless_safe() && !capabilities.headless_safe) {
            return false;
        }
        if (requires_native_surface() && !capabilities.native_surface) {
            return false;
        }
        return true;
    }
};

/// \brief Enumerates logical event types emitted by a window backend.
enum class EventType {
    /// No event. Used when polling fails to retrieve a value.
    None,
    /// The user requested the window to close (e.g., alt+F4 or pressing the
    /// close button).
    CloseRequested,
    /// The window client area has been resized. Payload is ResizeEvent.
    Resized,
    /// The window focus has changed. Payload is FocusEvent.
    FocusChanged,
    /// Implementation specific custom payload. Payload is std::string.
    Custom,
};

/// \brief Payload describing a resize event.
struct ResizeEvent {
    std::uint32_t width{0};
    std::uint32_t height{0};
};

/// \brief Payload describing a focus change.
struct FocusEvent {
    bool focused{false};
};

/// \brief Represents a window level event.
struct Event {
    using Payload = std::variant<std::monostate, ResizeEvent, FocusEvent, std::string>;

    EventType type{EventType::None};
    Payload payload{};

    /// Convenience helper constructing a close event.
    static Event close_requested() noexcept {
        return Event{EventType::CloseRequested, {}};
    }

    /// Convenience helper constructing a resize event.
    static Event resized(std::uint32_t width, std::uint32_t height) noexcept {
        return Event{EventType::Resized, ResizeEvent{width, height}};
    }

    /// Convenience helper constructing a focus event.
    static Event focus_changed(bool focused) noexcept {
        return Event{EventType::FocusChanged, FocusEvent{focused}};
    }

    /// Convenience helper constructing a custom payload event.
    static Event custom(std::string message) {
        return Event{EventType::Custom, std::move(message)};
    }
};

/// \brief Interface describing a queue of window events.
class EventQueue {
public:
    virtual ~EventQueue() noexcept = default;

    /// Pushes a new event into the queue. Implementations own the event copy
    /// and must remain valid until it is polled.
    virtual void push(Event event) = 0;

    /// Attempts to pop an event from the queue. Returns true when an event was
    /// available and assigns it to \p out_event.
    virtual bool poll(Event& out_event) = 0;

    /// Removes all pending events from the queue.
    virtual void clear() noexcept = 0;

    /// Returns true when no events are waiting to be processed.
    [[nodiscard]] virtual bool empty() const noexcept = 0;

    /// Returns the number of queued events. Intended for diagnostics and
    /// testing only.
    [[nodiscard]] virtual std::size_t size() const noexcept = 0;
};

/// \brief Interface representing a swapchain ready surface.
class SwapchainSurface {
public:
    virtual ~SwapchainSurface() noexcept = default;

    /// Identifies the renderer backend that produced the surface (e.g.,
    /// "vulkan", "d3d12").
    [[nodiscard]] virtual std::string_view renderer_backend() const noexcept = 0;

    /// Identifies the window backend used to create the surface. Helpful for
    /// logging and validation.
    [[nodiscard]] virtual std::string_view window_backend() const noexcept = 0;

    /// Returns the opaque native surface handle. The meaning of this pointer is
    /// backend specific.
    [[nodiscard]] virtual void* native_surface() const noexcept = 0;

    /// User supplied pointer forwarded when constructing the surface.
    [[nodiscard]] virtual void* user_data() const noexcept = 0;
};

/// \brief Configuration forwarded to swapchain surface creation routines.
struct SwapchainSurfaceRequest {
    /// Renderer backend identifier (e.g., "vulkan").
    std::string renderer_backend;

    /// Optional hook invoked by the platform layer to hand control to the
    /// rendering subsystem. The callback receives the request and the native
    /// window handle. Returning nullptr signals that the hook could not create
    /// the surface and that the platform layer should fall back to an internal
    /// stub implementation.
    using Hook = std::function<std::unique_ptr<SwapchainSurface>(
        const SwapchainSurfaceRequest&, void* native_window_handle)>;

    Hook hook{};

    /// Optional opaque pointer forwarded to the renderer. The platform layer
    /// stores it inside the fallback surface implementation when provided.
    void* user_data{nullptr};
};

/// \brief Abstract window interface exposed by the platform module.
class Window {
public:
    virtual ~Window() noexcept = default;

    /// Identifies the active backend responsible for this window.
    [[nodiscard]] virtual std::string_view backend_name() const noexcept = 0;

    /// Returns the configuration snapshot captured when the window was
    /// constructed.
    [[nodiscard]] virtual const WindowConfig& config() const noexcept = 0;

    /// Makes the window visible.
    virtual void show() = 0;

    /// Hides the window.
    virtual void hide() = 0;

    /// Reports the last visibility state requested by the application.
    [[nodiscard]] virtual bool is_visible() const noexcept = 0;

    /// Requests the window to close. Implementations enqueue a corresponding
    /// CloseRequested event.
    virtual void request_close() = 0;

    /// Reports whether the window received a close request. This flag is
    /// cleared automatically when a new frame of events is pumped.
    [[nodiscard]] virtual bool close_requested() const noexcept = 0;

    /// Allows synthetic events to be posted into the backend event stream.
    /// Real implementations typically call this from their OS callbacks while
    /// tests may use it to emulate native behaviour. Events posted here are
    /// expected to materialise after the next call to pump_events().
    virtual void post_event(Event event) = 0;

    /// Pumps pending backend events, forwarding them into the shared event
    /// queue. Implementations are responsible for translating native events
    /// into the abstract Event type.
    virtual void pump_events() = 0;

    /// Accessor to the shared event queue. Ownership remains with the window
    /// and is released automatically when the window is destroyed.
    [[nodiscard]] virtual EventQueue& event_queue() noexcept = 0;

    /// Const overload of event_queue().
    [[nodiscard]] virtual const EventQueue& event_queue() const noexcept = 0;

    /// Creates or acquires a swapchain surface for the given renderer backend.
    /// Implementations invoke the optional hook before falling back to an
    /// internal stub surface. The caller assumes ownership of the returned
    /// object and must destroy it before destroying the window.
    [[nodiscard]] virtual std::unique_ptr<SwapchainSurface> create_swapchain_surface(
        const SwapchainSurfaceRequest& request) = 0;
};

/// \brief Allocates an in-memory event queue. The caller assumes shared
/// ownership using std::shared_ptr semantics. Destroying the last reference
/// releases the underlying storage.
[[nodiscard]] ENGINE_PLATFORM_API std::shared_ptr<EventQueue> create_event_queue();

/// \brief Constructs a window using the requested backend and event queue.
/// The returned pointer follows shared ownership semantics. Destroying the
/// last reference releases the native resources held by the window as well as
/// the associated event queue. When \p backend is WindowBackend::Auto the
/// implementation selects the most suitable backend for the current build.
/// Set the environment variable `ENGINE_PLATFORM_WINDOW_BACKEND` to override
/// the automatic selection (accepted values: `auto`, `mock`, `glfw`, `sdl`).
[[nodiscard]] ENGINE_PLATFORM_API std::shared_ptr<Window> create_window(
    WindowConfig config,
    WindowBackend backend = WindowBackend::Auto,
    std::shared_ptr<EventQueue> event_queue = {});

}  // namespace engine::platform

