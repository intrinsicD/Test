#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <vector>
#include <array>

#include "engine/animation/api.hpp"
#include "engine/core/plugin/isubsystem_interface.hpp"
#include "engine/core/telemetry/schema.hpp"
#include "engine/core/threading/io_thread_pool.hpp"
#include "engine/compute/api.hpp"
#include "engine/geometry/api.hpp"
#include "engine/io/telemetry.hpp"
#include "engine/math/math.hpp"
#include "engine/physics/api.hpp"
#include "engine/runtime/subsystem_registry.hpp"
#include "engine/scene/validation.hpp"

#if ENGINE_ENABLE_ASSETS
#    include "engine/assets/mesh_asset.hpp"
#    include "engine/assets/point_cloud_asset.hpp"
#    include "engine/assets/validation.hpp"
#endif

#if ENGINE_ENABLE_RENDERING
#    include "engine/rendering/components.hpp"
#    include "engine/rendering/frame_graph.hpp"
#    include "engine/rendering/gpu_scheduler.hpp"
#    include "engine/rendering/runtime_submission.hpp"
#    include "engine/rendering/resources/resource_provider.hpp"
#    include "engine/rendering/pipeline/research_baseline.hpp"
#endif

#if defined(_WIN32)
#  if defined(ENGINE_RUNTIME_EXPORTS)
#    define ENGINE_RUNTIME_API __declspec(dllexport)
#  else
#    define ENGINE_RUNTIME_API __declspec(dllimport)
#  endif
#else
#  define ENGINE_RUNTIME_API
#endif

namespace engine::runtime
{
    [[nodiscard]] ENGINE_RUNTIME_API std::string_view module_name() noexcept;
    [[nodiscard]] ENGINE_RUNTIME_API std::size_t module_count() noexcept;
    [[nodiscard]] ENGINE_RUNTIME_API std::string_view module_name_at(std::size_t index) noexcept;

    struct ENGINE_RUNTIME_API runtime_frame_state
    {
        double simulation_time{0.0};
        animation::AnimationRigPose pose{};
        geometry::Aabb bounds{};
        std::vector<math::vec3> body_positions{};
        compute::ExecutionReport dispatch_report{};

        struct ENGINE_RUNTIME_API scene_node_state
        {
            std::string name{};
            math::Transform<float> transform{};
        };

        std::vector<scene_node_state> scene_nodes{};
    };

    struct ENGINE_RUNTIME_API RuntimeHostDependencies
    {
        animation::AnimationController controller{
            animation::make_linear_controller(animation::make_default_clip())
        };
        geometry::SurfaceMesh mesh{geometry::make_unit_quad()};
        animation::RigBinding binding{};
        physics::PhysicsWorld world{};
        std::string scene_name{"runtime.scene"};
        std::vector<std::shared_ptr<core::plugin::ISubsystemInterface>> subsystem_plugins{};
        std::shared_ptr<SubsystemRegistry> subsystem_registry{};
        std::vector<std::string> enabled_subsystems{};
        core::threading::IoThreadPoolConfig streaming_config{.worker_count = 2, .queue_capacity = 64, .enable = true};
        std::function<std::unique_ptr<compute::Dispatcher>()> dispatcher_factory{};
#if ENGINE_ENABLE_RENDERING
        rendering::components::RenderGeometry render_geometry{};
        std::string renderable_name{"runtime.renderable"};
#endif
#if ENGINE_ENABLE_ASSETS
        struct AssetStreamingProviders
        {
            assets::MeshCache* mesh_cache{nullptr};
            assets::PointCloudCache* point_cloud_cache{nullptr};
        };

        AssetStreamingProviders asset_streaming{};
#endif
    };

    struct ENGINE_RUNTIME_API StreamingMetrics
    {
        std::size_t worker_count{0};
        std::size_t queue_capacity{0};
        std::size_t pending_tasks{0};
        std::size_t active_workers{0};
        std::uint64_t total_enqueued{0};
        std::uint64_t total_executed{0};
        std::uint64_t streaming_pending{0};
        std::uint64_t streaming_loading{0};
        std::uint64_t streaming_total_requests{0};
        std::uint64_t streaming_total_completed{0};
        std::uint64_t streaming_total_failed{0};
        std::uint64_t streaming_total_cancelled{0};
        std::uint64_t streaming_total_rejected{0};
        std::array<std::uint64_t, io::geometry_io_error_count()> streaming_geometry_failures{};
        std::array<std::string_view, io::geometry_io_error_count()> streaming_geometry_failure_labels{};
    };

    struct ENGINE_RUNTIME_API RuntimeStageTiming
    {
        std::string name{};
        double last_ms{0.0};
        double average_ms{0.0};
        double max_ms{0.0};
        std::uint64_t sample_count{0};
    };

    struct ENGINE_RUNTIME_API HotReloadFailureSummary
    {
        std::string identifier{};
        std::string error{};
        std::string hint{};
    };

    struct ENGINE_RUNTIME_API HotReloadDiagnostics
    {
        std::uint64_t attempt_count{0};
        std::uint64_t failure_count{0};
        std::uint64_t cancelled_count{0};
        std::uint64_t rejected_count{0};
        std::uint64_t pending_count{0};
        std::uint64_t loading_count{0};
        std::uint64_t total_requests{0};
        std::string last_error{};
        std::string error_hint{};
        std::vector<HotReloadFailureSummary> recent_failures{};
    };

    struct ENGINE_RUNTIME_API RuntimeInitializationFailure
    {
        std::string runtime{};
        std::string subsystem{};
        std::string category{};
        std::string message{};
        double duration_ms{0.0};
    };

    struct ENGINE_RUNTIME_API RuntimeSubsystemTiming
    {
        std::string name{};
        double last_initialize_ms{0.0};
        double last_tick_ms{0.0};
        double last_shutdown_ms{0.0};
        double max_initialize_ms{0.0};
        double max_tick_ms{0.0};
        double max_shutdown_ms{0.0};
        std::uint64_t initialize_count{0};
        std::uint64_t tick_count{0};
        std::uint64_t shutdown_count{0};
        std::uint64_t initialize_failure_count{0};
        double last_initialize_failure_ms{0.0};
        std::string last_initialize_failure_category{};
        std::string last_initialize_failure_message{};
    };

    enum class SceneValidationAlertLevel : std::uint32_t
    {
        None = 0U,
        Warning = 1U,
        Critical = 2U,
    };

    struct ENGINE_RUNTIME_API RuntimeDiagnostics
    {
        std::uint64_t initialize_count{0};
        std::uint64_t initialize_failure_count{0};
        std::uint64_t shutdown_count{0};
        std::uint64_t tick_count{0};
        double last_initialize_ms{0.0};
        double last_shutdown_ms{0.0};
        double last_tick_ms{0.0};
        double max_initialize_ms{0.0};
        double max_shutdown_ms{0.0};
        double max_tick_ms{0.0};
        double average_tick_ms{0.0};
        StreamingMetrics streaming{};
        HotReloadDiagnostics hot_reload{};
        std::vector<RuntimeStageTiming> stage_timings{};
        std::vector<RuntimeSubsystemTiming> subsystem_timings{};
        scene::validation::HierarchyValidationReport scene_validation{};
        std::uint64_t scene_validation_failure_frame_count{0};
        std::uint64_t scene_validation_consecutive_failure_frames{0};
        std::uint64_t scene_validation_max_consecutive_failure_frames{0};
        double last_scene_validation_failure_simulation_time{-1.0};
        double last_scene_validation_failure_wall_seconds{-1.0};
        SceneValidationAlertLevel scene_validation_alert_level{SceneValidationAlertLevel::None};
        physics::CollisionTelemetry physics_collision{};
        core::telemetry::MetricSet metrics{};
        io::GeometryIoTelemetrySnapshot geometry_io{};
#if ENGINE_ENABLE_RENDERING
        std::string frame_graph_serialization{};
        std::vector<engine::rendering::ResourceEvent> frame_graph_events{};
#endif
#if ENGINE_ENABLE_ASSETS
        std::vector<assets::HandleValidationSnapshotEntry> handle_validation{};
#endif
        RuntimeInitializationFailure last_initialize_failure{};
        bool has_initialize_failure{false};
    };

    namespace detail
    {
        ENGINE_RUNTIME_API void update_scene_validation_alert_state(
            RuntimeDiagnostics& diagnostics,
            const scene::validation::HierarchyValidationReport& report,
            double simulation_time,
            double wall_seconds) noexcept;
    }

    class ENGINE_RUNTIME_API RuntimeHost
    {
    public:
        RuntimeHost();
        explicit RuntimeHost(RuntimeHostDependencies dependencies);
        RuntimeHost(RuntimeHost&&) noexcept;
        RuntimeHost& operator=(RuntimeHost&&) noexcept;
        RuntimeHost(const RuntimeHost&) = delete;
        RuntimeHost& operator=(const RuntimeHost&) = delete;
        ~RuntimeHost();

        void initialize();
        void shutdown() noexcept;
        [[nodiscard]] bool is_initialized() const noexcept;
        runtime_frame_state tick(double dt);
        [[nodiscard]] const geometry::SurfaceMesh& current_mesh() const;
        [[nodiscard]] const animation::AnimationRigPose& current_pose() const;
        [[nodiscard]] const std::vector<math::vec3>& body_positions() const;
        [[nodiscard]] const std::vector<std::string>& joint_names() const;
        [[nodiscard]] const compute::ExecutionReport& last_dispatch_report() const;
        [[nodiscard]] const std::vector<runtime_frame_state::scene_node_state>& scene_nodes() const;
        [[nodiscard]] double simulation_time() const noexcept;
        [[nodiscard]] std::span<const std::string_view> subsystem_names() const noexcept;
        [[nodiscard]] const RuntimeDiagnostics& diagnostics() const noexcept;

        void configure(RuntimeHostDependencies dependencies);

#if ENGINE_ENABLE_RENDERING
        using RenderSubmissionContext = rendering::RuntimeSubmissionContext;

        void submit_render_graph(RenderSubmissionContext& context);
        void configure_research_rendering(const rendering::ResearchBaselineOptions& options) noexcept;
#endif

#if ENGINE_ENABLE_ASSETS
        [[nodiscard]] assets::AssetLoadFuture<assets::MeshHandle>
        request_mesh_asset(const assets::AssetLoadRequest& request);
        [[nodiscard]] assets::AssetLoadFuture<assets::PointCloudHandle>
        request_point_cloud_asset(const assets::AssetLoadRequest& request);
        [[nodiscard]] assets::AssetLoadState mesh_asset_state(std::string_view identifier) const;
        [[nodiscard]] assets::AssetLoadState point_cloud_asset_state(std::string_view identifier) const;
#endif

    private:
        struct Impl;
        std::unique_ptr<Impl> impl_;
    };

    ENGINE_RUNTIME_API void initialize();
    ENGINE_RUNTIME_API void shutdown();
    ENGINE_RUNTIME_API void configure(RuntimeHostDependencies dependencies);
    ENGINE_RUNTIME_API void configure_with_default_subsystems();
    ENGINE_RUNTIME_API void configure_with_default_subsystems(std::span<const std::string_view> enabled_subsystems);
    ENGINE_RUNTIME_API runtime_frame_state tick(double dt);
    [[nodiscard]] ENGINE_RUNTIME_API const geometry::SurfaceMesh& current_mesh();
    [[nodiscard]] ENGINE_RUNTIME_API bool is_initialized() noexcept;
    [[nodiscard]] ENGINE_RUNTIME_API const animation::AnimationRigPose& current_pose();
    [[nodiscard]] ENGINE_RUNTIME_API const std::vector<math::vec3>& body_positions();
    [[nodiscard]] ENGINE_RUNTIME_API const std::vector<std::string>& joint_names();
    [[nodiscard]] ENGINE_RUNTIME_API const compute::ExecutionReport& last_dispatch_report();
    [[nodiscard]] ENGINE_RUNTIME_API const std::vector<runtime_frame_state::scene_node_state>& scene_nodes();
    [[nodiscard]] ENGINE_RUNTIME_API double simulation_time() noexcept;
    [[nodiscard]] ENGINE_RUNTIME_API std::vector<std::string> default_subsystem_names();
    [[nodiscard]] ENGINE_RUNTIME_API StreamingMetrics streaming_metrics() noexcept;
    [[nodiscard]] ENGINE_RUNTIME_API const RuntimeDiagnostics& diagnostics() noexcept;

#if ENGINE_ENABLE_ASSETS
    [[nodiscard]] ENGINE_RUNTIME_API assets::AssetLoadFuture<assets::MeshHandle>
    request_mesh_asset(const assets::AssetLoadRequest& request);
    [[nodiscard]] ENGINE_RUNTIME_API assets::AssetLoadFuture<assets::PointCloudHandle>
    request_point_cloud_asset(const assets::AssetLoadRequest& request);
    [[nodiscard]] ENGINE_RUNTIME_API assets::AssetLoadState mesh_asset_state(std::string_view identifier);
    [[nodiscard]] ENGINE_RUNTIME_API assets::AssetLoadState point_cloud_asset_state(std::string_view identifier);
#endif

#if ENGINE_ENABLE_RENDERING
ENGINE_RUNTIME_API void submit_render_graph(RuntimeHost::RenderSubmissionContext & context);
ENGINE_RUNTIME_API void configure_research_rendering(
    const rendering::ResearchBaselineOptions& options) noexcept;
#endif
} // namespace engine::runtime

extern "C" ENGINE_RUNTIME_API const char* engine_runtime_module_name() noexcept;
extern "C" ENGINE_RUNTIME_API std::size_t engine_runtime_module_count() noexcept;
extern "C" ENGINE_RUNTIME_API const char* engine_runtime_module_at(std::size_t index) noexcept;
extern "C" ENGINE_RUNTIME_API void engine_runtime_configure_with_default_modules() noexcept;
extern "C" ENGINE_RUNTIME_API void engine_runtime_configure_with_modules(
    const char* const* module_names,
    std::size_t count) noexcept;
extern "C" ENGINE_RUNTIME_API void engine_runtime_initialize() noexcept;
extern "C" ENGINE_RUNTIME_API void engine_runtime_shutdown() noexcept;
extern "C" ENGINE_RUNTIME_API void engine_runtime_tick(double dt) noexcept;
extern "C" ENGINE_RUNTIME_API std::size_t engine_runtime_body_count() noexcept;
extern "C" ENGINE_RUNTIME_API void engine_runtime_body_position(std::size_t index, float* out_position) noexcept;
extern "C" ENGINE_RUNTIME_API std::size_t engine_runtime_joint_count() noexcept;
extern "C" ENGINE_RUNTIME_API const char* engine_runtime_joint_name(std::size_t index) noexcept;
extern "C" ENGINE_RUNTIME_API void engine_runtime_joint_translation(std::size_t index, float* out_translation) noexcept;
extern "C" ENGINE_RUNTIME_API void engine_runtime_mesh_bounds(float* out_min, float* out_max) noexcept;
extern "C" ENGINE_RUNTIME_API std::size_t engine_runtime_dispatch_count() noexcept;
extern "C" ENGINE_RUNTIME_API const char* engine_runtime_dispatch_name(std::size_t index) noexcept;
extern "C" ENGINE_RUNTIME_API double engine_runtime_dispatch_duration(std::size_t index) noexcept;
extern "C" ENGINE_RUNTIME_API std::size_t engine_runtime_scene_node_count() noexcept;
extern "C" ENGINE_RUNTIME_API const char* engine_runtime_scene_node_name(std::size_t index) noexcept;
extern "C" ENGINE_RUNTIME_API void engine_runtime_scene_node_transform(
    std::size_t index,
    float* out_scale,
    float* out_rotation,
    float* out_translation) noexcept;

enum engine_runtime_research_shading_mode
{
    ENGINE_RUNTIME_RESEARCH_SHADING_MODE_FORWARD = 0,
    ENGINE_RUNTIME_RESEARCH_SHADING_MODE_DEFERRED = 1,
};

struct engine_runtime_research_rendering_options
{
    std::uint32_t width;
    std::uint32_t height;
    int shading_mode;
    std::uint8_t overlay_normals;
    std::uint8_t overlay_uv;
    std::uint8_t overlay_material;
    std::uint8_t overlay_light_volume;
};

extern "C" ENGINE_RUNTIME_API void engine_runtime_configure_research_rendering(
    const struct engine_runtime_research_rendering_options* options) noexcept;
constexpr std::size_t engine_runtime_streaming_geometry_failure_capacity =
    engine::io::geometry_io_error_count();

extern "C" ENGINE_RUNTIME_API std::size_t
engine_runtime_streaming_geometry_failure_capacity_value() noexcept;

struct engine_runtime_streaming_metrics
{
    std::size_t worker_count;
    std::size_t queue_capacity;
    std::size_t pending_tasks;
    std::size_t active_workers;
    std::uint64_t total_enqueued;
    std::uint64_t total_executed;
    std::uint64_t streaming_pending;
    std::uint64_t streaming_loading;
    std::uint64_t streaming_total_requests;
    std::uint64_t streaming_total_completed;
    std::uint64_t streaming_total_failed;
    std::uint64_t streaming_total_cancelled;
    std::uint64_t streaming_total_rejected;
    std::uint32_t streaming_geometry_failure_count;
    std::uint64_t streaming_geometry_failures[engine_runtime_streaming_geometry_failure_capacity];
    const char* streaming_geometry_failure_labels[engine_runtime_streaming_geometry_failure_capacity];
};

extern "C" ENGINE_RUNTIME_API void engine_runtime_streaming_metrics(
    struct engine_runtime_streaming_metrics* out_metrics) noexcept;
extern "C" ENGINE_RUNTIME_API void engine_runtime_diagnostic_streaming_metrics(
    struct engine_runtime_streaming_metrics* out_metrics) noexcept;

struct engine_runtime_hot_reload_metrics
{
    std::uint64_t attempt_count;
    std::uint64_t failure_count;
    std::uint64_t cancelled_count;
    std::uint64_t rejected_count;
    std::uint64_t pending_count;
    std::uint64_t loading_count;
    std::uint64_t total_requests;
    const char* last_error;
    const char* error_hint;
};

extern "C" ENGINE_RUNTIME_API void engine_runtime_diagnostic_hot_reload_metrics(
    struct engine_runtime_hot_reload_metrics* out_metrics) noexcept;
extern "C" ENGINE_RUNTIME_API std::uint32_t
engine_runtime_diagnostic_hot_reload_recent_failure_count() noexcept;
extern "C" ENGINE_RUNTIME_API const char*
engine_runtime_diagnostic_hot_reload_recent_failure_identifier(std::uint32_t index) noexcept;
extern "C" ENGINE_RUNTIME_API const char*
engine_runtime_diagnostic_hot_reload_recent_failure_error(std::uint32_t index) noexcept;
extern "C" ENGINE_RUNTIME_API const char*
engine_runtime_diagnostic_hot_reload_recent_failure_hint(std::uint32_t index) noexcept;

extern "C" ENGINE_RUNTIME_API std::uint64_t engine_runtime_diagnostic_initialize_count() noexcept;
extern "C" ENGINE_RUNTIME_API std::uint64_t engine_runtime_diagnostic_initialize_failure_count() noexcept;
extern "C" ENGINE_RUNTIME_API std::uint64_t engine_runtime_diagnostic_shutdown_count() noexcept;
extern "C" ENGINE_RUNTIME_API std::uint64_t engine_runtime_diagnostic_tick_count() noexcept;
extern "C" ENGINE_RUNTIME_API double engine_runtime_diagnostic_last_initialize_ms() noexcept;
extern "C" ENGINE_RUNTIME_API double engine_runtime_diagnostic_last_shutdown_ms() noexcept;
extern "C" ENGINE_RUNTIME_API double engine_runtime_diagnostic_last_tick_ms() noexcept;
extern "C" ENGINE_RUNTIME_API bool engine_runtime_diagnostic_has_initialize_failure() noexcept;
extern "C" ENGINE_RUNTIME_API const char* engine_runtime_diagnostic_last_initialize_failure_runtime() noexcept;
extern "C" ENGINE_RUNTIME_API const char* engine_runtime_diagnostic_last_initialize_failure_subsystem() noexcept;
extern "C" ENGINE_RUNTIME_API const char* engine_runtime_diagnostic_last_initialize_failure_category() noexcept;
extern "C" ENGINE_RUNTIME_API const char* engine_runtime_diagnostic_last_initialize_failure_message() noexcept;
extern "C" ENGINE_RUNTIME_API double engine_runtime_diagnostic_last_initialize_failure_duration_ms() noexcept;
extern "C" ENGINE_RUNTIME_API double engine_runtime_diagnostic_average_tick_ms() noexcept;
extern "C" ENGINE_RUNTIME_API double engine_runtime_diagnostic_max_tick_ms() noexcept;
extern "C" ENGINE_RUNTIME_API std::size_t engine_runtime_diagnostic_stage_count() noexcept;
extern "C" ENGINE_RUNTIME_API const char* engine_runtime_diagnostic_stage_name(std::size_t index) noexcept;
extern "C" ENGINE_RUNTIME_API double engine_runtime_diagnostic_stage_last_ms(std::size_t index) noexcept;
extern "C" ENGINE_RUNTIME_API double engine_runtime_diagnostic_stage_average_ms(std::size_t index) noexcept;
extern "C" ENGINE_RUNTIME_API double engine_runtime_diagnostic_stage_max_ms(std::size_t index) noexcept;
extern "C" ENGINE_RUNTIME_API std::uint64_t engine_runtime_diagnostic_stage_samples(std::size_t index) noexcept;
extern "C" ENGINE_RUNTIME_API std::size_t engine_runtime_diagnostic_subsystem_count() noexcept;
extern "C" ENGINE_RUNTIME_API const char* engine_runtime_diagnostic_subsystem_name(std::size_t index) noexcept;
extern "C" ENGINE_RUNTIME_API double engine_runtime_diagnostic_subsystem_last_initialize_ms(std::size_t index) noexcept;
extern "C" ENGINE_RUNTIME_API double engine_runtime_diagnostic_subsystem_last_tick_ms(std::size_t index) noexcept;
extern "C" ENGINE_RUNTIME_API double engine_runtime_diagnostic_subsystem_last_shutdown_ms(std::size_t index) noexcept;
extern "C" ENGINE_RUNTIME_API std::uint64_t engine_runtime_diagnostic_subsystem_initialize_count(
    std::size_t index) noexcept;
extern "C" ENGINE_RUNTIME_API std::uint64_t engine_runtime_diagnostic_subsystem_tick_count(std::size_t index) noexcept;
extern "C" ENGINE_RUNTIME_API std::uint64_t engine_runtime_diagnostic_subsystem_shutdown_count(
    std::size_t index) noexcept;
extern "C" ENGINE_RUNTIME_API std::uint64_t engine_runtime_diagnostic_subsystem_initialize_failure_count(
    std::size_t index) noexcept;
extern "C" ENGINE_RUNTIME_API double engine_runtime_diagnostic_subsystem_max_initialize_ms(std::size_t index) noexcept;
extern "C" ENGINE_RUNTIME_API double engine_runtime_diagnostic_subsystem_max_tick_ms(std::size_t index) noexcept;
extern "C" ENGINE_RUNTIME_API double engine_runtime_diagnostic_subsystem_max_shutdown_ms(std::size_t index) noexcept;
extern "C" ENGINE_RUNTIME_API double engine_runtime_diagnostic_subsystem_last_initialize_failure_ms(
    std::size_t index) noexcept;
extern "C" ENGINE_RUNTIME_API const char* engine_runtime_diagnostic_subsystem_last_initialize_failure_category(
    std::size_t index) noexcept;
extern "C" ENGINE_RUNTIME_API const char* engine_runtime_diagnostic_subsystem_last_initialize_failure_message(
    std::size_t index) noexcept;
extern "C" ENGINE_RUNTIME_API std::uint64_t engine_runtime_diagnostic_scene_issue_count() noexcept;
extern "C" ENGINE_RUNTIME_API std::uint64_t engine_runtime_diagnostic_scene_cycle_count() noexcept;
extern "C" ENGINE_RUNTIME_API std::uint64_t engine_runtime_diagnostic_scene_dangling_parent_count() noexcept;
extern "C" ENGINE_RUNTIME_API std::uint64_t engine_runtime_diagnostic_scene_missing_parent_hierarchy_count() noexcept;
extern "C" ENGINE_RUNTIME_API std::uint64_t engine_runtime_diagnostic_scene_non_finite_transform_count() noexcept;
extern "C" ENGINE_RUNTIME_API std::uint64_t engine_runtime_diagnostic_scene_transform_mismatch_count() noexcept;
extern "C" ENGINE_RUNTIME_API std::size_t engine_runtime_diagnostic_scene_issue_total() noexcept;
extern "C" ENGINE_RUNTIME_API std::uint32_t engine_runtime_diagnostic_scene_issue_entity(std::size_t index) noexcept;
extern "C" ENGINE_RUNTIME_API std::uint32_t engine_runtime_diagnostic_scene_issue_related(std::size_t index) noexcept;
extern "C" ENGINE_RUNTIME_API const char* engine_runtime_diagnostic_scene_issue_type_name(std::size_t index) noexcept;
extern "C" ENGINE_RUNTIME_API const char* engine_runtime_diagnostic_scene_issue_message(std::size_t index) noexcept;
extern "C" ENGINE_RUNTIME_API std::size_t engine_runtime_diagnostic_metric_count() noexcept;
extern "C" ENGINE_RUNTIME_API const char* engine_runtime_diagnostic_metric_name(std::size_t index) noexcept;
extern "C" ENGINE_RUNTIME_API int engine_runtime_diagnostic_metric_kind(std::size_t index) noexcept;
extern "C" ENGINE_RUNTIME_API int engine_runtime_diagnostic_metric_unit(std::size_t index) noexcept;
extern "C" ENGINE_RUNTIME_API const char* engine_runtime_diagnostic_metric_description(std::size_t index) noexcept;
extern "C" ENGINE_RUNTIME_API std::size_t engine_runtime_diagnostic_metric_label_count(std::size_t index) noexcept;
extern "C" ENGINE_RUNTIME_API const char* engine_runtime_diagnostic_metric_label_key(std::size_t metric_index,
    std::size_t label_index) noexcept;
extern "C" ENGINE_RUNTIME_API const char* engine_runtime_diagnostic_metric_label_value(std::size_t metric_index,
    std::size_t label_index) noexcept;
extern "C" ENGINE_RUNTIME_API bool engine_runtime_diagnostic_metric_is_integral(std::size_t index) noexcept;
extern "C" ENGINE_RUNTIME_API double engine_runtime_diagnostic_metric_value(std::size_t index) noexcept;
extern "C" ENGINE_RUNTIME_API std::int64_t engine_runtime_diagnostic_metric_value_int(std::size_t index) noexcept;