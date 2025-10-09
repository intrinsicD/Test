set(CMAKE_SYSTEM_NAME Windows)

set(CMAKE_C_COMPILER cl CACHE FILEPATH "MSVC C compiler" FORCE)
set(CMAKE_CXX_COMPILER cl CACHE FILEPATH "MSVC C++ compiler" FORCE)
set(CMAKE_GENERATOR_TOOLSET "host=x64" CACHE STRING "Prefer 64-bit host tools" FORCE)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON CACHE BOOL "Export compilation database" FORCE)
set(ENGINE_THIRD_PARTY_DIR "${CMAKE_SOURCE_DIR}/third_party" CACHE PATH "Pinned third-party source directory" FORCE)
