set(ENGINE_MODULES
    animation
    assets
    compute
    core
    geometry
    io
    physics
    platform
    rendering
    scene
    runtime
)

foreach(module ${ENGINE_MODULES})
    add_subdirectory(${module})
endforeach()
