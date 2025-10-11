#include <gtest/gtest.h>

#include "engine/io/importers/animation.hpp"

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

    engine::io::animation::save_clip(clip, path);

    EXPECT_EQ(engine::io::animation::ClipFormat::json, engine::io::animation::detect_clip_format(path));

    const auto loaded = engine::io::animation::load_clip(path);
    EXPECT_EQ(clip.name, loaded.name);
    EXPECT_NEAR(clip.duration, loaded.duration, 1e-6);

    std::filesystem::remove(path);
}

TEST(AnimationImporter, ThrowsWhenFormatUnknown)
{
    const auto path = make_temporary_path("engine_animation_clip.unknown");
    std::ofstream stream{path};
    stream << "not json";

    EXPECT_THROW(engine::io::animation::load_clip(path), std::runtime_error);

    std::filesystem::remove(path);
}
