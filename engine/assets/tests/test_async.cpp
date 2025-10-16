#include <gtest/gtest.h>

#include "engine/assets/async.hpp"
#include "engine/assets/handles.hpp"

#include <chrono>
#include <filesystem>
#include <thread>

namespace
{
    using namespace std::chrono_literals;
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

