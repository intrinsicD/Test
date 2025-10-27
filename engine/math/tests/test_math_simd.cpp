#include <array>
#include <cmath>
#include <cstddef>
#include <random>

#include <gtest/gtest.h>

#include "engine/math/vector.hpp"

namespace
{
    template <typename T, std::size_t N, std::size_t Width>
    struct SimdVector
    {
        std::array<std::array<T, Width>, N> components{};

        constexpr std::array<T, Width>& operator[](std::size_t index) noexcept
        {
            return components[index];
        }

        constexpr const std::array<T, Width>& operator[](std::size_t index) const noexcept
        {
            return components[index];
        }
    };

    template <typename T, std::size_t N, std::size_t Width>
    SimdVector<T, N, Width> LoadVectors(const std::array<engine::math::Vector<T, N>, Width>& values) noexcept
    {
        SimdVector<T, N, Width> result{};
        for (std::size_t lane = 0; lane < Width; ++lane)
        {
            for (std::size_t component = 0; component < N; ++component)
            {
                result[component][lane] = values[lane][component];
            }
        }
        return result;
    }

    template <typename T, std::size_t N, std::size_t Width>
    std::array<engine::math::Vector<T, N>, Width> StoreVectors(const SimdVector<T, N, Width>& pack) noexcept
    {
        std::array<engine::math::Vector<T, N>, Width> result{};
        for (std::size_t lane = 0; lane < Width; ++lane)
        {
            for (std::size_t component = 0; component < N; ++component)
            {
                result[lane][component] = pack[component][lane];
            }
        }
        return result;
    }

    template <typename T, std::size_t N, std::size_t Width>
    SimdVector<T, N, Width> SimdAdd(const SimdVector<T, N, Width>& lhs, const SimdVector<T, N, Width>& rhs) noexcept
    {
        SimdVector<T, N, Width> result{};
        for (std::size_t lane = 0; lane < Width; ++lane)
        {
            for (std::size_t component = 0; component < N; ++component)
            {
                result[component][lane] = lhs[component][lane] + rhs[component][lane];
            }
        }
        return result;
    }

    template <typename T, std::size_t N, std::size_t Width>
    SimdVector<T, N, Width> SimdSubtract(const SimdVector<T, N, Width>& lhs,
                                         const SimdVector<T, N, Width>& rhs) noexcept
    {
        SimdVector<T, N, Width> result{};
        for (std::size_t lane = 0; lane < Width; ++lane)
        {
            for (std::size_t component = 0; component < N; ++component)
            {
                result[component][lane] = lhs[component][lane] - rhs[component][lane];
            }
        }
        return result;
    }

    template <typename T, std::size_t N, std::size_t Width>
    SimdVector<T, N, Width> SimdScale(const SimdVector<T, N, Width>& value,
                                      const std::array<T, Width>& scalars) noexcept
    {
        SimdVector<T, N, Width> result{};
        for (std::size_t lane = 0; lane < Width; ++lane)
        {
            for (std::size_t component = 0; component < N; ++component)
            {
                result[component][lane] = value[component][lane] * scalars[lane];
            }
        }
        return result;
    }

    template <typename T, std::size_t N, std::size_t Width>
    std::array<T, Width> SimdDot(const SimdVector<T, N, Width>& lhs, const SimdVector<T, N, Width>& rhs) noexcept
    {
        std::array<T, Width> result{};
        for (std::size_t lane = 0; lane < Width; ++lane)
        {
            T value = T(0);
            for (std::size_t component = 0; component < N; ++component)
            {
                value += lhs[component][lane] * rhs[component][lane];
            }
            result[lane] = value;
        }
        return result;
    }

    template <typename T, std::size_t Width>
    SimdVector<T, 3, Width> SimdCross(const SimdVector<T, 3, Width>& lhs,
                                      const SimdVector<T, 3, Width>& rhs) noexcept
    {
        SimdVector<T, 3, Width> result{};
        for (std::size_t lane = 0; lane < Width; ++lane)
        {
            const T x = lhs[1][lane] * rhs[2][lane] - lhs[2][lane] * rhs[1][lane];
            const T y = lhs[2][lane] * rhs[0][lane] - lhs[0][lane] * rhs[2][lane];
            const T z = lhs[0][lane] * rhs[1][lane] - lhs[1][lane] * rhs[0][lane];
            result[0][lane] = x;
            result[1][lane] = y;
            result[2][lane] = z;
        }
        return result;
    }

    template <typename T, std::size_t Width>
    std::array<T, Width> SimdLength(const SimdVector<T, 3, Width>& value) noexcept
    {
        std::array<T, Width> result{};
        for (std::size_t lane = 0; lane < Width; ++lane)
        {
            const T x = value[0][lane];
            const T y = value[1][lane];
            const T z = value[2][lane];
            result[lane] = static_cast<T>(std::sqrt(static_cast<double>(x * x + y * y + z * z)));
        }
        return result;
    }

    template <typename T, std::size_t Width>
    SimdVector<T, 3, Width> SimdNormalize(const SimdVector<T, 3, Width>& value) noexcept
    {
        SimdVector<T, 3, Width> result{};
        for (std::size_t lane = 0; lane < Width; ++lane)
        {
            const T x = value[0][lane];
            const T y = value[1][lane];
            const T z = value[2][lane];
            const T length = static_cast<T>(std::sqrt(static_cast<double>(x * x + y * y + z * z)));
            if (length == T(0))
            {
                result[0][lane] = x;
                result[1][lane] = y;
                result[2][lane] = z;
            }
            else
            {
                const T inv_length = T(1) / length;
                result[0][lane] = x * inv_length;
                result[1][lane] = y * inv_length;
                result[2][lane] = z * inv_length;
            }
        }
        return result;
    }

    template <typename T>
    void ExpectVec3Near(const engine::math::Vector<T, 3>& value,
                        const engine::math::Vector<T, 3>& expected,
                        T tolerance)
    {
        for (std::size_t component = 0; component < 3; ++component)
        {
            EXPECT_NEAR(value[component], expected[component], tolerance);
        }
    }

    template <std::size_t Width>
    struct SimdWidthTag
    {
        static constexpr std::size_t value = Width;
    };

    template <typename WidthTag>
    class SimdValidationTest : public ::testing::Test
    {
    protected:
        static constexpr std::size_t kWidth = WidthTag::value;
        static constexpr std::size_t kBatchSize = 32;

        using Vec3 = engine::math::Vector<float, 3>;
        using SimdVec3 = SimdVector<float, 3, kWidth>;

        static std::array<Vec3, kBatchSize> MakeVectors(std::uint32_t seed)
        {
            std::mt19937 rng(seed);
            std::uniform_real_distribution<float> distribution(-10.0F, 10.0F);
            std::array<Vec3, kBatchSize> values{};
            for (auto& value : values)
            {
                value = Vec3{distribution(rng), distribution(rng), distribution(rng)};
            }
            values[0] = Vec3{};
            return values;
        }

        static std::array<float, kBatchSize> MakeScalars(std::uint32_t seed)
        {
            std::mt19937 rng(seed);
            std::uniform_real_distribution<float> distribution(0.25F, 2.0F);
            std::array < float, kBatchSize > values{};
            for (auto& value : values)
            {
                value = distribution(rng);
            }
            return values;
        }

        template <typename Fn>
        static void ForEachChunk(const std::array<Vec3, kBatchSize>& lhs,
                                 const std::array<Vec3, kBatchSize>& rhs,
                                 Fn&& fn)
        {
            static_assert(kBatchSize % kWidth == 0, "batch size must be a multiple of the SIMD width");
            for (std::size_t offset = 0; offset < kBatchSize; offset += kWidth)
            {
                std::array<Vec3, kWidth> lhs_chunk{};
                std::array<Vec3, kWidth> rhs_chunk{};
                for (std::size_t lane = 0; lane < kWidth; ++lane)
                {
                    lhs_chunk[lane] = lhs[offset + lane];
                    rhs_chunk[lane] = rhs[offset + lane];
                }
                fn(lhs_chunk, rhs_chunk);
            }
        }

        template <typename Fn>
        static void ForEachChunk(const std::array<Vec3, kBatchSize>& lhs,
                                 const std::array<Vec3, kBatchSize>& rhs,
                                 const std::array<float, kBatchSize>& scalars,
                                 Fn&& fn)
        {
            static_assert(kBatchSize % kWidth == 0, "batch size must be a multiple of the SIMD width");
            for (std::size_t offset = 0; offset < kBatchSize; offset += kWidth)
            {
                std::array<Vec3, kWidth> lhs_chunk{};
                std::array<Vec3, kWidth> rhs_chunk{};
                std::array < float, kWidth > scalar_chunk{};
                for (std::size_t lane = 0; lane < kWidth; ++lane)
                {
                    lhs_chunk[lane] = lhs[offset + lane];
                    rhs_chunk[lane] = rhs[offset + lane];
                    scalar_chunk[lane] = scalars[offset + lane];
                }
                fn(lhs_chunk, rhs_chunk, scalar_chunk);
            }
        }
    };

    using SimdWidths = ::testing::Types<SimdWidthTag<4>, SimdWidthTag<8>>;
    TYPED_TEST_SUITE(SimdValidationTest, SimdWidths);
} // namespace

TYPED_TEST(SimdValidationTest, VectorArithmeticMatchesScalar)
{
    constexpr std::size_t width = TestFixture::kWidth;
    const auto lhs = TestFixture::MakeVectors(42U);
    const auto rhs = TestFixture::MakeVectors(1337U);
    const auto scalars = TestFixture::MakeScalars(2025U);

    TestFixture::ForEachChunk(lhs, rhs, scalars,
                              [](const auto& lhs_chunk, const auto& rhs_chunk, const auto& scalar_chunk)
                              {
                                  const auto lhs_pack = LoadVectors<float, 3, width>(lhs_chunk);
                                  const auto rhs_pack = LoadVectors<float, 3, width>(rhs_chunk);
                                  const auto add_pack = SimdAdd(lhs_pack, rhs_pack);
                                  const auto sub_pack = SimdSubtract(lhs_pack, rhs_pack);
                                  const auto scale_pack = SimdScale(lhs_pack, scalar_chunk);

                                  const auto add_vectors = StoreVectors(add_pack);
                                  const auto sub_vectors = StoreVectors(sub_pack);
                                  const auto scaled_vectors = StoreVectors(scale_pack);

                                  for (std::size_t lane = 0; lane < width; ++lane)
                                  {
                                      const auto expected_add = lhs_chunk[lane] + rhs_chunk[lane];
                                      const auto expected_sub = lhs_chunk[lane] - rhs_chunk[lane];
                                      const auto expected_scale = lhs_chunk[lane] * scalar_chunk[lane];

                                      ExpectVec3Near(add_vectors[lane], expected_add, 1e-5F);
                                      ExpectVec3Near(sub_vectors[lane], expected_sub, 1e-5F);
                                      ExpectVec3Near(scaled_vectors[lane], expected_scale, 1e-5F);
                                  }
                              });
}

TYPED_TEST(SimdValidationTest, DotAndCrossMatchScalar)
{
    constexpr std::size_t width = TestFixture::kWidth;
    const auto lhs = TestFixture::MakeVectors(84U);
    const auto rhs = TestFixture::MakeVectors(2112U);

    TestFixture::ForEachChunk(lhs, rhs,
                              [](const auto& lhs_chunk, const auto& rhs_chunk)
                              {
                                  const auto lhs_pack = LoadVectors<float, 3, width>(lhs_chunk);
                                  const auto rhs_pack = LoadVectors<float, 3, width>(rhs_chunk);

                                  const auto dot_values = SimdDot(lhs_pack, rhs_pack);
                                  const auto cross_pack = SimdCross(lhs_pack, rhs_pack);
                                  const auto cross_vectors = StoreVectors(cross_pack);

                                  for (std::size_t lane = 0; lane < width; ++lane)
                                  {
                                      const auto expected_dot = engine::math::dot(lhs_chunk[lane], rhs_chunk[lane]);
                                      const auto expected_cross = engine::math::cross(lhs_chunk[lane], rhs_chunk[lane]);

                                      EXPECT_NEAR(dot_values[lane], expected_dot, 1e-5F);
                                      ExpectVec3Near(cross_vectors[lane], expected_cross, 1e-5F);
                                  }
                              });
}

TYPED_TEST(SimdValidationTest, NormalizeMatchesScalar)
{
    constexpr std::size_t width = TestFixture::kWidth;
    const auto values = TestFixture::MakeVectors(17U);

    TestFixture::ForEachChunk(values, values,
                              [](const auto& value_chunk, const auto&)
                              {
                                  const auto pack = LoadVectors<float, 3, width>(value_chunk);
                                  const auto lengths = SimdLength(pack);
                                  const auto normalized_pack = SimdNormalize(pack);
                                  const auto normalized_vectors = StoreVectors(normalized_pack);

                                  for (std::size_t lane = 0; lane < width; ++lane)
                                  {
                                      const auto expected_length = engine::math::length(value_chunk[lane]);
                                      const auto expected_normalized = engine::math::normalize(value_chunk[lane]);

                                      EXPECT_NEAR(lengths[lane], expected_length, 1e-5F);
                                      ExpectVec3Near(normalized_vectors[lane], expected_normalized, 1e-5F);

                                      if (expected_length > 0.0F)
                                      {
                                          const auto normalized_length = engine::math::length(normalized_vectors[lane]);
                                          EXPECT_NEAR(normalized_length, 1.0F, 1e-5F);
                                      }
                                  }
                              });
}