include_guard(GLOBAL)

# Vulkan Memory Allocator (header-only interface)
# Prefer local copy under third_party/vma/include/vk_mem_alloc.h; fallback to fetching repo
set(_LOCAL_VMA_DIR "${CMAKE_CURRENT_SOURCE_DIR}/vma")
set(_VMA_GIT_TAG v3.1.0)

if(EXISTS "${_LOCAL_VMA_DIR}/include/vk_mem_alloc.h")
    add_library(vma INTERFACE)
    target_include_directories(vma INTERFACE "${_LOCAL_VMA_DIR}/include")
else()
    FetchContent_Declare(vma_external
            GIT_REPOSITORY https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator.git
            GIT_TAG ${_VMA_GIT_TAG}
            GIT_SHALLOW TRUE
    )
    FetchContent_MakeAvailable(vma_external)
    add_library(vma INTERFACE)
    target_include_directories(vma INTERFACE "${vma_external_SOURCE_DIR}")
endif()
if(NOT TARGET VMA::VMA)
    add_library(VMA::VMA ALIAS vma)
endif()

unset(_LOCAL_VMA_DIR)
unset(_VMA_GIT_TAG)