if(NOT DEFINED ENGINE_MODULES)
    message(FATAL_ERROR "ENGINE_MODULES is not defined. Include the engine directory from the root CMake configuration.")
endif()

foreach(module IN LISTS ENGINE_MODULES)
    add_subdirectory(${module})
endforeach()
