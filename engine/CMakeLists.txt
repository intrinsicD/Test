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
)

foreach(module ${ENGINE_MODULES})
    add_subdirectory(${module})
endforeach()
