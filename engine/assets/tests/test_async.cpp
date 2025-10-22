#include <gtest/gtest.h>

#include "engine/assets/async.hpp"
#include "engine/assets/handles.hpp"
#include "engine/assets/mesh_asset.hpp"
#include "engine/core/threading/io_thread_pool.hpp"
#include "engine/io/telemetry.hpp"

#include <atomic>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <future>
#include <thread>

namespace
{
    using namespace std::chrono_literals;

    class IoThreadPoolScope
    {
    public:
        IoThreadPoolScope(std::size_t worker_count, std::size_t queue_capacity, bool enable = true)
        {
            engine::core::threading::IoThreadPool::instance().configure(
                {.worker_count = worker_count, .queue_capacity = queue_capacity, .enable = enable});
        }

        IoThreadPoolScope(const IoThreadPoolScope&) = delete;
        IoThreadPoolScope& operator=(const IoThreadPoolScope&) = delete;

        ~IoThreadPoolScope()
        {
            engine::core::threading::IoThreadPool::instance().shutdown();
        }
    };
}

TEST(AssetLoadRequest, FromPathAssignsDefaults)
{
    engine::assets::AssetImportParams params;
    auto request = engine::assets::AssetLoadRequest::from_path(
        engine::assets::AssetType::mesh, std::filesystem::path{"/tmp/example.obj"}, params,
        engine::assets::AssetLoadPriority::High, 250ms, true);

    EXPECT_EQ(request.type, engine::assets::AssetType::mesh);
    EXPECT_EQ(request.identifier, std::filesystem::path{"/tmp/example.obj"}.generic_string());
    EXPECT_EQ(request.priority, engine::assets::AssetLoadPriority::High);
    EXPECT_TRUE(request.has_deadline());
    EXPECT_TRUE(request.allow_blocking_fallback);
    EXPECT_EQ(request.import_params.format_hint, ".obj");
}

TEST(AssetLoadFuture, ReportsProgressAndResult)
{
    auto [promise, future] = engine::assets::detail::make_asset_load_channel<engine::assets::MeshHandle>();

    EXPECT_TRUE(future.valid());
    EXPECT_EQ(future.state(), engine::assets::AssetLoadState::Pending);

    promise.set_loading();
    EXPECT_EQ(promise.state(), engine::assets::AssetLoadState::Loading);

    engine::assets::ProgressInfo info;
    info.total_bytes = 100;
    info.bytes_transferred = 40;
    promise.update_progress(info);
    EXPECT_NEAR(future.progress().completion_ratio(), 0.4, 1e-6);

    engine::assets::MeshHandle handle{std::string{"mesh/test"}};

    std::thread worker([&]() {
        std::this_thread::sleep_for(10ms);
        promise.set_ready(handle);
    });

    future.wait();
    const auto result = future.get();
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value().id(), "mesh/test");
    EXPECT_EQ(future.state(), engine::assets::AssetLoadState::Ready);

    worker.join();
}

TEST(AssetLoadFuture, CancelRequestIsPropagated)
{
    auto [promise, future] = engine::assets::detail::make_asset_load_channel<engine::assets::MeshHandle>();

    EXPECT_FALSE(future.cancellation_requested());
    future.cancel();
    EXPECT_TRUE(future.cancellation_requested());

    std::thread worker([&]() {
        EXPECT_TRUE(promise.cancellation_requested());
        promise.set_cancelled();
    });

    const auto result = future.get();
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code(), engine::assets::AssetLoadErrorCategory::Cancelled);
    EXPECT_EQ(future.state(), engine::assets::AssetLoadState::Cancelled);

    worker.join();
}

TEST(AssetLoadFuture, FailurePropagatesErrors)
{
    auto [promise, future] = engine::assets::detail::make_asset_load_channel<engine::assets::MeshHandle>();

    std::thread worker([&]() {
        promise.set_loading();
        promise.set_failed(engine::assets::make_asset_load_error(
            engine::assets::AssetLoadErrorCategory::DecodeError, "decode failure"));
    });

    const auto result = future.get();
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code(), engine::assets::AssetLoadErrorCategory::DecodeError);
    EXPECT_EQ(std::string{result.error().message()}, "decode failure");

    worker.join();
}

class MeshCacheAsyncTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        engine::core::threading::IoThreadPool::instance().configure({.worker_count = 2, .queue_capacity = 8, .enable = true});
    }

    void TearDown() override
    {
        engine::core::threading::IoThreadPool::instance().shutdown();
    }
};

namespace
{
    std::filesystem::path write_temporary_obj()
    {
        auto path = std::filesystem::temp_directory_path() / "engine_async_mesh.obj";
        std::ofstream stream{path};
        stream << "o mesh\n";
        stream << "v 0 0 0\n";
        stream << "v 1 0 0\n";
        stream << "v 0 1 0\n";
        stream << "f 1 2 3\n";
        stream.close();
        return path;
    }

    std::filesystem::path write_corrupted_obj()
    {
        auto path = std::filesystem::temp_directory_path() / "engine_async_mesh_corrupted.obj";
        std::ofstream stream{path};
        stream << "o mesh\n";
        stream << "v 0 0 0\n";
        stream << "v 1 0 0\n";
        stream << "f 1 3 2\n";  // references a missing vertex index
        stream.close();
        return path;
    }
}

TEST_F(MeshCacheAsyncTest, LoadAsyncCompletesSuccessfully)
{
    engine::assets::MeshCache cache;
    const auto path = write_temporary_obj();
    auto request = engine::assets::AssetLoadRequest::from_path(
        engine::assets::AssetType::mesh, path, {});

    auto future = cache.load_async(request, engine::core::threading::IoThreadPool::instance());
    future.wait();
    const auto result = future.get();
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(cache.async_state(request.identifier), engine::assets::AssetLoadState::Ready);

    std::filesystem::remove(path);
}

TEST_F(MeshCacheAsyncTest, LoadAsyncReportsFailures)
{
    engine::assets::MeshCache cache;
    auto request = engine::assets::AssetLoadRequest::from_identifier(
        engine::assets::AssetType::mesh, "/tmp/non-existent-mesh.obj");

    auto future = cache.load_async(request, engine::core::threading::IoThreadPool::instance());
    future.wait();
    const auto result = future.get();
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(cache.async_state(request.identifier), engine::assets::AssetLoadState::Failed);
}

TEST_F(MeshCacheAsyncTest, LoadAsyncPropagatesGeometryErrorOnDecodeFailure)
{
    engine::assets::MeshCache cache;
    const auto path = write_corrupted_obj();
    auto request = engine::assets::AssetLoadRequest::from_path(
        engine::assets::AssetType::mesh, path, {});

    auto future = cache.load_async(request, engine::core::threading::IoThreadPool::instance());
    future.wait();
    const auto result = future.get();
    ASSERT_FALSE(result.has_value());
    ASSERT_TRUE(result.error().geometry_error().has_value());
    EXPECT_EQ(result.error().geometry_error()->code(), engine::io::GeometryIoError::invalid_argument);
    EXPECT_EQ(cache.async_state(request.identifier), engine::assets::AssetLoadState::Failed);

    std::filesystem::remove(path);
}

TEST_F(MeshCacheAsyncTest, LoadAsyncHonoursCancellation)
{
    engine::assets::MeshCache cache;
    const auto path = write_temporary_obj();
    auto request = engine::assets::AssetLoadRequest::from_path(
        engine::assets::AssetType::mesh, path, {});

    auto future = cache.load_async(request, engine::core::threading::IoThreadPool::instance());
    future.cancel();
    future.wait();
    const auto result = future.get();
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code(), engine::assets::AssetLoadErrorCategory::Cancelled);
    EXPECT_EQ(cache.async_state(request.identifier), engine::assets::AssetLoadState::Cancelled);

    std::filesystem::remove(path);
}

TEST(AssetStreamingTelemetry, RecordsSuccessfulTransition)
{
    auto& telemetry = engine::assets::AssetStreamingTelemetry::instance();
    telemetry.reset_for_testing();

    IoThreadPoolScope scope{0, 4};
    engine::assets::MeshCache cache;

    const auto path = write_temporary_obj();
    auto request = engine::assets::AssetLoadRequest::from_path(
        engine::assets::AssetType::mesh, path, {});
    request.allow_blocking_fallback = true;

    auto future = cache.load_async(request, engine::core::threading::IoThreadPool::instance());
    future.wait();
    const auto result = future.get();
    ASSERT_TRUE(result.has_value());

    const auto snapshot = telemetry.snapshot();
    EXPECT_EQ(snapshot.total_requests, 1U);
    EXPECT_EQ(snapshot.total_completed, 1U);
    EXPECT_EQ(snapshot.total_failed, 0U);
    EXPECT_EQ(snapshot.total_cancelled, 0U);
    EXPECT_EQ(snapshot.total_rejected, 0U);
    EXPECT_EQ(snapshot.pending, 0U);
    EXPECT_EQ(snapshot.loading, 0U);
    for (const auto failure_count : snapshot.geometry_failures)
    {
        EXPECT_EQ(failure_count, 0U);
    }

    std::filesystem::remove(path);
}

TEST(AssetStreamingTelemetry, RecordsFailureTransition)
{
    auto& telemetry = engine::assets::AssetStreamingTelemetry::instance();
    telemetry.reset_for_testing();

    IoThreadPoolScope scope{0, 4};
    engine::assets::MeshCache cache;

    auto request = engine::assets::AssetLoadRequest::from_identifier(
        engine::assets::AssetType::mesh, "/tmp/does-not-exist.obj");
    request.allow_blocking_fallback = true;

    auto future = cache.load_async(request, engine::core::threading::IoThreadPool::instance());
    future.wait();
    const auto result = future.get();
    ASSERT_FALSE(result.has_value());
    ASSERT_TRUE(result.error().geometry_error().has_value());
    EXPECT_EQ(result.error().geometry_error()->code(), engine::io::GeometryIoError::file_not_found);
    EXPECT_EQ(result.error().code(), engine::assets::AssetLoadErrorCategory::IoFailure);

    const auto snapshot = telemetry.snapshot();
    EXPECT_EQ(snapshot.total_requests, 1U);
    EXPECT_EQ(snapshot.total_completed, 0U);
    EXPECT_EQ(snapshot.total_failed, 1U);
    EXPECT_EQ(snapshot.total_cancelled, 0U);
    EXPECT_EQ(snapshot.total_rejected, 0U);
    EXPECT_EQ(snapshot.pending, 0U);
    EXPECT_EQ(snapshot.loading, 0U);
    const auto failure_index = engine::io::geometry_io_error_index(engine::io::GeometryIoError::file_not_found);
    ASSERT_LT(failure_index, snapshot.geometry_failures.size());
    EXPECT_EQ(snapshot.geometry_failures[failure_index], 1U);
}

TEST(AssetStreamingTelemetry, RecordsDecodeFailureTransition)
{
    auto& telemetry = engine::assets::AssetStreamingTelemetry::instance();
    telemetry.reset_for_testing();

    IoThreadPoolScope scope{0, 4};
    engine::assets::MeshCache cache;

    const auto path = write_corrupted_obj();
    auto request = engine::assets::AssetLoadRequest::from_path(
        engine::assets::AssetType::mesh, path, {});
    request.allow_blocking_fallback = true;

    auto future = cache.load_async(request, engine::core::threading::IoThreadPool::instance());
    future.wait();
    const auto result = future.get();
    ASSERT_FALSE(result.has_value());
    ASSERT_TRUE(result.error().geometry_error().has_value());
    EXPECT_EQ(result.error().geometry_error()->code(), engine::io::GeometryIoError::invalid_argument);

    const auto snapshot = telemetry.snapshot();
    EXPECT_EQ(snapshot.total_requests, 1U);
    EXPECT_EQ(snapshot.total_completed, 0U);
    EXPECT_EQ(snapshot.total_failed, 1U);
    EXPECT_EQ(snapshot.total_cancelled, 0U);
    EXPECT_EQ(snapshot.total_rejected, 0U);
    EXPECT_EQ(snapshot.pending, 0U);
    EXPECT_EQ(snapshot.loading, 0U);
    const auto failure_index =
        engine::io::geometry_io_error_index(engine::io::GeometryIoError::invalid_argument);
    ASSERT_LT(failure_index, snapshot.geometry_failures.size());
    EXPECT_EQ(snapshot.geometry_failures[failure_index], 1U);

    std::filesystem::remove(path);
}

TEST(AssetStreamingTelemetry, RecordsCancellationTransition)
{
    auto& telemetry = engine::assets::AssetStreamingTelemetry::instance();
    telemetry.reset_for_testing();

    IoThreadPoolScope scope{1, 4};
    engine::assets::AssetAsyncQueue<engine::assets::MeshHandle> queue;

    auto future = queue.schedule(
        "cancel",
        engine::assets::AssetLoadPriority::Normal,
        false,
        [](engine::assets::detail::AssetLoadPromise<engine::assets::MeshHandle>& promise) {
            engine::assets::MeshHandle handle{std::string{"cancel"}};
            for (int i = 0; i < 50; ++i)
            {
                if (promise.cancellation_requested())
                {
                    break;
                }
                std::this_thread::sleep_for(2ms);
            }
            return engine::assets::AssetLoadResult<engine::assets::MeshHandle>{handle};
        },
        engine::core::threading::IoThreadPool::instance());

    std::this_thread::sleep_for(5ms);
    future.cancel();
    future.wait();
    const auto result = future.get();
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code(), engine::assets::AssetLoadErrorCategory::Cancelled);

    const auto snapshot = telemetry.snapshot();
    EXPECT_EQ(snapshot.total_requests, 1U);
    EXPECT_EQ(snapshot.total_completed, 0U);
    EXPECT_EQ(snapshot.total_failed, 0U);
    EXPECT_EQ(snapshot.total_cancelled, 1U);
    EXPECT_EQ(snapshot.total_rejected, 0U);
    EXPECT_EQ(snapshot.pending, 0U);
    EXPECT_EQ(snapshot.loading, 0U);
    for (const auto failure_count : snapshot.geometry_failures)
    {
        EXPECT_EQ(failure_count, 0U);
    }
}

TEST(AssetStreamingTelemetry, RecordsRejectedEnqueue)
{
    auto& telemetry = engine::assets::AssetStreamingTelemetry::instance();
    telemetry.reset_for_testing();

    IoThreadPoolScope scope{0, 1};
    engine::assets::AssetAsyncQueue<engine::assets::MeshHandle> queue;

    auto future = queue.schedule(
        "reject",
        engine::assets::AssetLoadPriority::Normal,
        false,
        [](engine::assets::detail::AssetLoadPromise<engine::assets::MeshHandle>&) {
            engine::assets::MeshHandle handle{std::string{"reject"}};
            return engine::assets::AssetLoadResult<engine::assets::MeshHandle>{handle};
        },
        engine::core::threading::IoThreadPool::instance());

    future.wait();
    const auto result = future.get();
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code(), engine::assets::AssetLoadErrorCategory::Timeout);

    const auto snapshot = telemetry.snapshot();
    EXPECT_EQ(snapshot.total_requests, 1U);
    EXPECT_EQ(snapshot.total_completed, 0U);
    EXPECT_EQ(snapshot.total_failed, 1U);
    EXPECT_EQ(snapshot.total_cancelled, 0U);
    EXPECT_EQ(snapshot.total_rejected, 1U);
    EXPECT_EQ(snapshot.pending, 0U);
    EXPECT_EQ(snapshot.loading, 0U);
    for (const auto failure_count : snapshot.geometry_failures)
    {
        EXPECT_EQ(failure_count, 0U);
    }
}

TEST(AssetAsyncQueue, CancelPendingRequestResolvesFuture)
{
    auto& telemetry = engine::assets::AssetStreamingTelemetry::instance();
    telemetry.reset_for_testing();

    IoThreadPoolScope scope{1, 4};
    engine::assets::AssetAsyncQueue<engine::assets::MeshHandle> queue;

    std::promise<void> release_blocker;
    std::promise<void> blocker_started;

    auto blocking_future = queue.schedule(
        "blocking",
        engine::assets::AssetLoadPriority::High,
        false,
        [&release_blocker, &blocker_started](engine::assets::detail::AssetLoadPromise<engine::assets::MeshHandle>&)
            -> engine::assets::AssetLoadResult<engine::assets::MeshHandle> {
            blocker_started.set_value();
            release_blocker.get_future().wait();
            engine::assets::MeshHandle handle{std::string{"blocking"}};
            return engine::assets::AssetLoadResult<engine::assets::MeshHandle>{handle};
        },
        engine::core::threading::IoThreadPool::instance());

    blocker_started.get_future().wait();

    std::atomic<bool> cancelled_task_executed{false};

    auto cancellable_future = queue.schedule(
        "cancel-pending",
        engine::assets::AssetLoadPriority::Low,
        false,
        [&cancelled_task_executed](engine::assets::detail::AssetLoadPromise<engine::assets::MeshHandle>&)
            -> engine::assets::AssetLoadResult<engine::assets::MeshHandle> {
            cancelled_task_executed.store(true, std::memory_order_relaxed);
            engine::assets::MeshHandle handle{std::string{"cancelled"}};
            return engine::assets::AssetLoadResult<engine::assets::MeshHandle>{handle};
        },
        engine::core::threading::IoThreadPool::instance());

    cancellable_future.cancel();
    const auto result = cancellable_future.get();
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code(), engine::assets::AssetLoadErrorCategory::Cancelled);
    EXPECT_FALSE(cancelled_task_executed.load(std::memory_order_relaxed));

    release_blocker.set_value();
    blocking_future.wait();
    const auto completed = blocking_future.get();
    ASSERT_TRUE(completed.has_value());
    EXPECT_EQ(completed.value().id(), "blocking");
}

TEST(AssetAsyncQueue, CancelDuringExecutionResolvesAsCancelled)
{
    auto& telemetry = engine::assets::AssetStreamingTelemetry::instance();
    telemetry.reset_for_testing();

    IoThreadPoolScope scope{1, 4};
    engine::assets::AssetAsyncQueue<engine::assets::MeshHandle> queue;

    std::promise<void> task_started;
    std::promise<void> allow_exit;

    auto future = queue.schedule(
        "cancel-during-execution",
        engine::assets::AssetLoadPriority::Normal,
        false,
        [&task_started, &allow_exit](engine::assets::detail::AssetLoadPromise<engine::assets::MeshHandle>& promise)
            -> engine::assets::AssetLoadResult<engine::assets::MeshHandle> {
            task_started.set_value();
            allow_exit.get_future().wait();

            while (!promise.cancellation_requested())
            {
                std::this_thread::sleep_for(1ms);
            }

            auto error = engine::assets::make_asset_load_error(
                engine::assets::AssetLoadErrorCategory::Cancelled, "request cancelled mid-execution");
            promise.set_cancelled(error);
            return engine::assets::AssetLoadResult<engine::assets::MeshHandle>{error};
        },
        engine::core::threading::IoThreadPool::instance());

    task_started.get_future().wait();
    future.cancel();
    allow_exit.set_value();

    future.wait();
    const auto result = future.get();
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code(), engine::assets::AssetLoadErrorCategory::Cancelled);

    const auto snapshot = telemetry.snapshot();
    EXPECT_EQ(snapshot.total_requests, 1U);
    EXPECT_EQ(snapshot.total_completed, 0U);
    EXPECT_EQ(snapshot.total_failed, 0U);
    EXPECT_EQ(snapshot.total_cancelled, 1U);
    EXPECT_EQ(snapshot.total_rejected, 0U);
}

