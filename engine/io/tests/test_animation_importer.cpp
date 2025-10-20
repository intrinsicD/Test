#include <gtest/gtest.h>

#include "engine/io/importers/animation.hpp"

#include "engine/io/errors.hpp"

#include <filesystem>
#include <fstream>

namespace
{
std::filesystem::path make_temporary_path(const std::string& stem)
{
    auto path = std::filesystem::temp_directory_path() / stem;
    std::filesystem::create_directories(path.parent_path());
    return path;
}
} // namespace

TEST(AnimationImporter, DetectsJsonClipsByExtension)
{
    const auto clip = engine::animation::make_default_clip();
    const auto path = make_temporary_path("engine_animation_clip.anim.json");

    ASSERT_TRUE(engine::io::animation::save_clip(clip, path));

    const auto detection = engine::io::animation::detect_clip_format(path);
    ASSERT_TRUE(detection);
    EXPECT_EQ(engine::io::animation::ClipFormat::json, detection.value());

    const auto loaded = engine::io::animation::load_clip(path);
    ASSERT_TRUE(loaded);
    EXPECT_EQ(clip.name, loaded.value().name);
    EXPECT_NEAR(clip.duration, loaded.value().duration, 1e-6);

    std::filesystem::remove(path);
}

TEST(AnimationImporter, ThrowsWhenFormatUnknown)
{
    const auto path = make_temporary_path("engine_animation_clip.unknown");
    std::ofstream stream{path};
    stream << "not json";

    const auto loaded = engine::io::animation::load_clip(path);
    ASSERT_FALSE(loaded);
    EXPECT_EQ(engine::io::AnimationIoError::unsupported_format, loaded.error().code());

    std::filesystem::remove(path);
}
