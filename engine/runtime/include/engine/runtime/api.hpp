#pragma once

#include <cstddef>
#include <string_view>
#include <vector>

#include "engine/animation/api.hpp"
#include "engine/compute/api.hpp"
#include "engine/geometry/api.hpp"
#include "engine/math/math.hpp"
#include "engine/physics/api.hpp"

#if defined(_WIN32)
#  if defined(ENGINE_RUNTIME_EXPORTS)
#    define ENGINE_RUNTIME_API __declspec(dllexport)
#  else
#    define ENGINE_RUNTIME_API __declspec(dllimport)
#  endif
#else
#  define ENGINE_RUNTIME_API
#endif

namespace engine::runtime {

[[nodiscard]] std::string_view module_name() noexcept;
[[nodiscard]] std::size_t module_count() noexcept;
[[nodiscard]] std::string_view module_name_at(std::size_t index) noexcept;

struct runtime_frame_state {
    double simulation_time{0.0};
    animation::AnimationRigPose pose{};
    geometry::MeshBounds bounds{};
    std::vector<math::vec3> body_positions{};
    compute::ExecutionReport dispatch_report{};
};

ENGINE_RUNTIME_API void initialize();
ENGINE_RUNTIME_API void shutdown();
ENGINE_RUNTIME_API runtime_frame_state tick(double dt);
[[nodiscard]] ENGINE_RUNTIME_API const geometry::SurfaceMesh& current_mesh();

}  // namespace engine::runtime

extern "C" ENGINE_RUNTIME_API const char* engine_runtime_module_name() noexcept;
extern "C" ENGINE_RUNTIME_API std::size_t engine_runtime_module_count() noexcept;
extern "C" ENGINE_RUNTIME_API const char* engine_runtime_module_at(std::size_t index) noexcept;
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

