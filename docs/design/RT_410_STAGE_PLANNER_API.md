# RT-410 Stage Planner API Design

**Status:** Draft  
**Task:** RT-410-A  
**Owner:** runtime-lead  
**Date:** 2025-11-06  

## Executive Summary

This document specifies the API contracts for the runtime stage planner, presentation backends, and synchronization surfaces that enable deterministic GPU presentation across backends (OpenGL, Vulkan, Mock) and tooling integration.

Building on the existing `RuntimeStagePlanner` foundation in `engine/runtime/include/engine/runtime/runtime_loop_plan.hpp`, this design extends the runtime to support backend-aware presentation hooks, telemetry integration, and tooling reuse.

## Goals

1. **Deterministic presentation:** Stage planner executes presentation adapters with consistent ordering and telemetry across headless and interactive modes
2. **Backend abstraction:** Shared presentation contract supports OpenGL, Vulkan, and Mock without runtime-specific wiring
3. **Tooling integration:** Presentation adapters expose synchronization handles for tooling preview and diagnostics
4. **Performance transparency:** Per-stage telemetry captures presentation latency without regressing frame budgets

## Non-Goals

- Asynchronous stage execution (deferred to future work)
- Multi-window presentation (single primary surface for Phase 1)
- Platform-specific presentation extensions (XR, HDR, etc.)

## Background

### Current State

From ADR-0008 and existing implementation:

- `RuntimeStagePlanner` exists and iterates `RuntimeLoopPlan` stages
- `RuntimeStageHandle` provides stage metadata (name, phase, thread affinity)
- `RuntimeStageExecution` pairs handle + function for execution
- `PresentationBackend` interface exists but lacks integration with stage planner

**Gap:** Runtime doesn't expose presentation-specific stage coordination or synchronization primitives for tooling.

### References

- [`docs/specs/ADR_0008_RUNTIME_MAIN_LOOP_AND_TOOLING.md`](../specs/ADR_0008_RUNTIME_MAIN_LOOP_AND_TOOLING.md)
- [`engine/runtime/include/engine/runtime/runtime_loop_plan.hpp`](../../engine/runtime/include/engine/runtime/runtime_loop_plan.hpp)
- [`engine/rendering/include/engine/rendering/presentation_backend.hpp`](../../engine/rendering/include/engine/rendering/presentation_backend.hpp)

## API Specification

### 1. Enhanced Stage Metadata

Extend `RuntimeStageHandle` to include presentation-specific budget and telemetry:

```cpp
namespace engine::runtime
{
    /// Stage execution budget hint for scheduling and telemetry
    struct StageBudget
    {
        double target_milliseconds{0.0};  ///< Target execution time
        bool enforce_budget{false};        ///< Emit warning if exceeded
    };

    /// Enhanced stage metadata with presentation budgets
    struct RuntimeStageHandle
    {
        std::string_view name{};
        RuntimeLoopPhase phase{RuntimeLoopPhase::Simulation};
        RuntimeLoopThreadAffinity thread_affinity{RuntimeLoopThreadAffinity::MainThread};
        bool record_in_execution_report{true};
        std::size_t index{0};
        StageBudget budget{};  ///< NEW: Optional execution budget
    };
}
```

### 2. Presentation Configuration

Define presentation-specific configuration passed to backends:

```cpp
namespace engine::rendering
{
    /// Backend capability flags
    enum class PresentationCapability : uint32_t
    {
        None = 0,
        Vsync = 1 << 0,
        TripleBuffering = 1 << 1,
        ImGuiOverlay = 1 << 2,
        Readback = 1 << 3,
        Telemetry = 1 << 4,
    };

    /// Presentation mode selection
    enum class PresentationMode
    {
        Headless,      ///< No display output, CPU readback available
        Interactive,   ///< Display output with vsync
        FastPreview,   ///< Display output without vsync for tooling
    };

    /// Configuration for presentation backend initialization
    struct PresentationConfig
    {
        PresentationMode mode{PresentationMode::Interactive};
        uint32_t width{1920};
        uint32_t height{1080};
        PresentationCapability capabilities{PresentationCapability::ImGuiOverlay};
        bool enable_telemetry{true};
        void* native_window_handle{nullptr};  ///< Platform-specific window
    };

    /// Frame presentation payload
    struct PresentationFrame
    {
        void* render_target{nullptr};          ///< Backend-specific render target
        void* imgui_draw_data{nullptr};        ///< ImGui draw data (nullable)
        uint64_t frame_number{0};
        double delta_seconds{0.0};
    };
}
```

### 3. Presentation Backend Interface (Enhanced)

Extend existing `PresentationBackend` with initialization and lifecycle:

```cpp
namespace engine::rendering
{
    class PresentationBackend
    {
    public:
        virtual ~PresentationBackend() = default;

        /// Initialize backend with presentation configuration
        /// \return Success or error code (InvalidConfig, UnsupportedCapability, etc.)
        virtual RuntimeResult<void> initialize(const PresentationConfig& config) = 0;

        /// Present the current frame
        virtual void present(const RuntimePresentationContext& context) = 0;

        /// NEW: Present with explicit frame payload
        virtual RuntimeResult<void> present_frame(const PresentationFrame& frame) = 0;

        /// Query backend capabilities
        [[nodiscard]] virtual PresentationCapability supported_capabilities() const = 0;

        /// Shutdown and release resources
        virtual void shutdown() = 0;

        /// Optional: CPU readback for headless/testing
        virtual RuntimeResult<void> readback(void* cpu_buffer, size_t buffer_size) const
        {
            return RuntimeError::NotImplemented;
        }
    };
}
```

### 4. Presentation Stage Coordination

New runtime utilities for coordinating presentation stages:

```cpp
namespace engine::runtime
{
    /// Synchronization handle for presentation timing
    struct PresentationSyncHandle
    {
        uint64_t frame_number{0};
        double cpu_start_time_ms{0.0};
        double gpu_start_time_ms{0.0};
        void* backend_fence{nullptr};  ///< Backend-specific sync primitive
    };

    /// Context passed to presentation stages
    struct PresentationStageContext
    {
        RuntimeHost& host;
        double delta_seconds{0.0};
        PresentationSyncHandle sync{};
    };

    /// Presentation stage configuration builder
    class PresentationStageBuilder
    {
    public:
        /// Configure presentation backend
        PresentationStageBuilder& with_backend(
            std::unique_ptr<rendering::PresentationBackend> backend);

        /// Set presentation budget
        PresentationStageBuilder& with_budget(double target_ms);

        /// Enable telemetry capture
        PresentationStageBuilder& with_telemetry(bool enabled);

        /// Build presentation stage
        [[nodiscard]] RuntimeResult<RuntimeLoopStage> build() const;

    private:
        std::unique_ptr<rendering::PresentationBackend> backend_{};
        double budget_ms_{16.67};  // Default 60fps
        bool telemetry_enabled_{true};
    };
}
```

### 5. Integration with RuntimeStagePlanner

Usage pattern for configuring presentation in the runtime loop:

```cpp
// Example: Configure runtime with presentation stage
RuntimeLoopBuilder builder;

// Add standard stages (simulation, rendering, etc.)
builder.add_stage("Input", RuntimeLoopPhase::Simulation, input_fn);
builder.add_stage("Animation", RuntimeLoopPhase::Simulation, animation_fn, {"Input"});
builder.add_stage("Rendering", RuntimeLoopPhase::Simulation, rendering_fn, {"Animation"});

// Build presentation stage with OpenGL backend
auto presentation_stage = PresentationStageBuilder()
    .with_backend(std::make_unique<OpenGLPresentationBackend>())
    .with_budget(16.67)  // 60fps target
    .with_telemetry(true)
    .build();

if (presentation_stage.is_ok()) {
    builder.add_stage("Present", 
                     RuntimeLoopPhase::Presentation, 
                     presentation_stage.value().function,
                     {"Rendering"});
}

auto plan = builder.build();
RuntimeStagePlanner planner;
planner.configure_plan(plan.value());

// Execute loop
while (running) {
    planner.reset_iteration();
    while (auto stage = planner.next_stage()) {
        if (stage.value().has_value()) {
            auto& exec = stage.value().value();
            exec.function(delta_time);
        }
    }
}
```

## Concrete Backend Implementations

### OpenGL Presentation Backend

```cpp
class OpenGLPresentationBackend : public PresentationBackend
{
public:
    RuntimeResult<void> initialize(const PresentationConfig& config) override;
    void present(const RuntimePresentationContext& context) override;
    RuntimeResult<void> present_frame(const PresentationFrame& frame) override;
    PresentationCapability supported_capabilities() const override;
    void shutdown() override;

private:
    GLFWwindow* window_{nullptr};
    PresentationConfig config_{};
};
```

**Key Responsibilities:**
- GLFW window management
- OpenGL context creation and swap chain
- ImGui rendering via OpenGL3 backend
- Vsync configuration

### Vulkan Presentation Backend

```cpp
class VulkanPresentationBackend : public PresentationBackend
{
public:
    RuntimeResult<void> initialize(const PresentationConfig& config) override;
    void present(const RuntimePresentationContext& context) override;
    RuntimeResult<void> present_frame(const PresentationFrame& frame) override;
    PresentationCapability supported_capabilities() const override;
    void shutdown() override;

private:
    VkSwapchainKHR swapchain_{VK_NULL_HANDLE};
    std::vector<VkSemaphore> present_semaphores_{};
    PresentationConfig config_{};
};
```

**Key Responsibilities:**
- Vulkan WSI swapchain management
- Timeline semaphores for GPU/CPU sync
- ImGui rendering via Vulkan backend
- Presentation queue submission

### Mock/Headless Backend

```cpp
class MockPresentationBackend : public PresentationBackend
{
public:
    RuntimeResult<void> initialize(const PresentationConfig& config) override;
    void present(const RuntimePresentationContext& context) override;
    RuntimeResult<void> present_frame(const PresentationFrame& frame) override;
    PresentationCapability supported_capabilities() const override;
    void shutdown() override;
    RuntimeResult<void> readback(void* cpu_buffer, size_t buffer_size) const override;

private:
    std::vector<uint8_t> readback_buffer_{};
    PresentationConfig config_{};
};
```

**Key Responsibilities:**
- No-op presentation for CI/testing
- Optional CPU readback for validation
- Frame counting and telemetry only

## Telemetry Integration

### Per-Stage Metrics

Automatically captured by `RuntimeStagePlanner`:

```cpp
struct StageTelemetry
{
    std::string_view stage_name;
    RuntimeLoopPhase phase;
    double start_time_ms;
    double end_time_ms;
    double duration_ms;
    double budget_ms;
    bool budget_exceeded;
};
```

### Presentation-Specific Metrics

Additional metrics for presentation stages:

```cpp
struct PresentationTelemetry
{
    uint64_t frame_number;
    double cpu_frame_time_ms;
    double gpu_frame_time_ms;
    double present_wait_time_ms;
    PresentationMode mode;
    bool vsync_enabled;
};
```

### Telemetry Export

Metrics exported to existing telemetry infrastructure:

```json
{
  "frame": 12345,
  "stages": [
    {"name": "Input", "phase": "Simulation", "duration_ms": 0.42, "budget_ms": 1.0},
    {"name": "Animation", "phase": "Simulation", "duration_ms": 2.15, "budget_ms": 3.0},
    {"name": "Rendering", "phase": "Simulation", "duration_ms": 8.73, "budget_ms": 12.0},
    {"name": "Present", "phase": "Presentation", "duration_ms": 1.24, "budget_ms": 2.0}
  ],
  "presentation": {
    "cpu_frame_time_ms": 12.54,
    "gpu_frame_time_ms": 11.82,
    "present_wait_time_ms": 0.58,
    "mode": "Interactive",
    "vsync": true
  }
}
```

## Tooling Integration Patterns

### Sandbox Preview

```cpp
// Sandbox can inject custom presentation backend for preview
class SandboxPreviewBackend : public PresentationBackend
{
    // Renders to ImGui texture for embedded preview
    RuntimeResult<void> present_frame(const PresentationFrame& frame) override;
};

// Configure runtime with sandbox backend
auto backend = std::make_unique<SandboxPreviewBackend>(preview_texture);
auto stage = PresentationStageBuilder()
    .with_backend(std::move(backend))
    .build();
```

### Diagnostics Capture

```cpp
// Capture frames for diagnostics without interactive window
class DiagnosticsCaptureBackend : public PresentationBackend
{
    RuntimeResult<void> readback(void* cpu_buffer, size_t buffer_size) const override;
    
    void save_frame_to_disk(const std::string& path);
};
```

### Python Harness Integration

```python
# Python bindings for headless execution
import engine3g

runtime = engine3g.RuntimeHost()
backend = engine3g.MockPresentationBackend(width=1920, height=1080)

# Configure presentation
config = engine3g.PresentationConfig(
    mode=engine3g.PresentationMode.Headless,
    enable_telemetry=True
)
backend.initialize(config)

# Run and capture
for frame in range(100):
    runtime.tick()
    telemetry = backend.get_telemetry()
    print(f"Frame {frame}: {telemetry.cpu_frame_time_ms}ms")
```

### Scripting Synchronization Hooks

Lightweight tooling does not need to link against the full C++ runtime to observe stage planner
state. Two C exports provide the necessary synchronization context:

```c
bool engine_runtime_presentation_stage_active(void);
const char* engine_runtime_loop_plan_serialization(void);
```

`engine_runtime_presentation_stage_active()` returns `true` when the presentation stage is wired
into the active loop plan (either via a callback or a presentation backend). Scripts can check this
flag before attempting screenshot capture or overlay composition. The serialization helper mirrors
the JSON produced by `RuntimeDiagnostics::loop_plan_serialization`, allowing harnesses to validate
stage ordering without parsing binary telemetry snapshots.

Python bindings in `engine3g.Loader` expose these exports as
`EngineRuntimeHandle.presentation_stage_active()` and `.loop_plan_serialization()`. Example:

```python
with engine3g.loader().runtime_session() as session:
    runtime = session.runtime
    if runtime.presentation_stage_active():
        plan_json = runtime.loop_plan_serialization()
        print("Presentation enabled; loop plan:")
        print(plan_json)
    else:
        print("Headless execution — skipping presentation capture")
```

## Edge Cases & Error Handling

### Backend Capability Mismatch

**Scenario:** Requested capabilities not supported by backend

```cpp
auto result = backend->initialize(config);
if (result.is_err()) {
    if (result.error() == RuntimeError::UnsupportedCapability) {
        // Fall back to mock backend
        backend = std::make_unique<MockPresentationBackend>();
        result = backend->initialize(fallback_config);
    }
}
```

### Synchronization Deadlock

**Mitigation:** Timeline semaphores with timeout + validation in sanitizer builds

```cpp
// Vulkan backend waits with timeout
VkSemaphoreWaitInfo wait_info{/* ... */};
wait_info.pSemaphores = &frame_semaphore;
uint64_t timeout_ns = 5'000'000'000;  // 5 seconds

if (vkWaitSemaphores(device, &wait_info, timeout_ns) == VK_TIMEOUT) {
    // Log deadlock and gracefully degrade
    log_error("Presentation semaphore timeout - possible deadlock");
    return RuntimeError::PresentationTimeout;
}
```

### Headless Mode Regression

**Validation:** Ensure headless backend doesn't require window/display

```cpp
TEST(PresentationBackend, HeadlessDoesNotRequireWindow) {
    MockPresentationBackend backend;
    PresentationConfig config{
        .mode = PresentationMode::Headless,
        .native_window_handle = nullptr  // Must work with null
    };
    
    auto result = backend.initialize(config);
    ASSERT_TRUE(result.is_ok());
}
```

### Telemetry Overload

**Protection:** Sample presentation telemetry, don't emit every frame

```cpp
class PresentationTelemetrySampler
{
    void record_frame(const PresentationTelemetry& telemetry) {
        if (frame_count_ % sample_rate_ == 0) {
            emit_to_diagnostics(telemetry);
        }
        frame_count_++;
    }

private:
    uint64_t frame_count_{0};
    uint64_t sample_rate_{60};  // Sample once per second at 60fps
};
```

## Implementation Checklist

### Phase 1: API Definition (RT-410-A)
- [x] Document stage planner API contracts (this document)
- [x] Define `PresentationConfig`, `PresentationFrame` structures
- [x] Specify `PresentationBackend` interface extensions
- [x] Design `PresentationStageBuilder` utility
- [ ] Review API with module leads
- [x] Update parent RT-410 with progress

### Phase 2: Core Implementation (RT-410-B)
- [ ] Implement `PresentationStageBuilder`
- [x] Extend `RuntimeStageHandle` with budget support
- [ ] Add presentation telemetry structures
- [x] Update `RuntimeStagePlanner` to capture stage budgets

### Phase 3: Backend Implementations (RT-410-C/D/E)
- [ ] Implement `OpenGLPresentationBackend` (wrap existing)
- [ ] Implement `VulkanPresentationBackend` (new)
- [ ] Implement `MockPresentationBackend` (headless)
- [ ] Add backend selection logic in RuntimeHost

### Phase 4: Integration & Testing (RT-410-F)
- [ ] Unit tests for stage planner coordination
- [ ] Integration test: OpenGL presentation with ImGui
- [ ] Integration test: Headless execution with readback
- [ ] Sanitizer validation for sync primitives
- [ ] Performance baseline: <2% overhead target

### Phase 5: Documentation (RT-410-G)
- [ ] Update ADR-0008 with implementation notes
- [ ] Module README updates (runtime, rendering)
- [ ] Sample code demonstrating backend configuration
- [ ] Python bindings documentation

## Open Questions

1. **Async stage execution:** Defer to follow-up ADR after single-threaded validation?
   - **Decision:** Yes, keep Phase 1 deterministic single-threaded

2. **UI state persistence:** How should panel layouts integrate with presentation?
   - **Decision:** Deferred to TL-310 (editor foundations) - presentation provides hooks only

3. **Screenshot capture API:** Expose via scripting or require loop-stage adapters?
   - **Decision:** Provide `readback()` method on backend, harness scripting can use directly

4. **Multi-backend support:** Can runtime switch backends at runtime?
   - **Decision:** Phase 1 requires restart to change backend; hot-swap deferred

## Success Criteria

- [ ] Stage planner executes presentation adapters with deterministic ordering
- [ ] OpenGL, Vulkan, and Mock backends share common `PresentationBackend` interface
- [ ] Telemetry captures per-stage and presentation-specific metrics
- [ ] Tooling can inject custom presentation backends for preview
- [ ] Headless mode works without window/display dependencies
- [ ] Performance overhead ≤2% vs baseline (measured in PM-510 demos)
- [ ] API design reviewed and approved by runtime-lead, rendering-lead, tools-lead

## References

- [ADR-0008: Runtime Main Loop and Tooling Integration](../specs/ADR_0008_RUNTIME_MAIN_LOOP_AND_TOOLING.md)
- [Runtime Loop Plan Header](../../engine/runtime/include/engine/runtime/runtime_loop_plan.hpp)
- [Presentation Backend Header](../../engine/rendering/include/engine/rendering/presentation_backend.hpp)
- [RT-410 Parent Task](../../hybrid_workflow/backlog/archive/RT-410-runtime-stage-planner.md)
- [TL-310 Editor Foundations](../../hybrid_workflow/backlog/TL-310-editor-foundations.md)
- [PM-510 Weekly Demos](../../hybrid_workflow/backlog/PM-510-weekly-integration-demos.md)

