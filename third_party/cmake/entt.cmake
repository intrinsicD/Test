include_guard(GLOBAL)

# EnTT (header-only)
set(_LOCAL_ENTT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/entt")
set(_ENTT_GIT_TAG v3.13.0)

if(EXISTS "${_LOCAL_ENTT_DIR}/CMakeLists.txt")
    add_subdirectory("${_LOCAL_ENTT_DIR}" "${CMAKE_BINARY_DIR}/third_party/entt")
else()
    FetchContent_Declare(entt_external
            GIT_REPOSITORY https://github.com/skypjack/entt.git
            GIT_TAG ${_ENTT_GIT_TAG}
            GIT_SHALLOW TRUE
    )
    FetchContent_MakeAvailable(entt_external)
endif()

unset(_LOCAL_ENTT_DIR)
unset(_ENTT_GIT_TAG)