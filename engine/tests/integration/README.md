# Engine Tests Integration

## Animation Integration Tests

### animation_io_integration
- **Purpose:** Validate clip loading through IO module
- **Test:** Load clip from file, validate structure, evaluate pose
- **Modules:** animation, io

### animation_runtime_integration
- **Purpose:** Validate animation drives runtime transforms
- **Test:** Advance blend tree, check scene transforms updated
- **Modules:** animation, runtime, scene

### animation_geometry_integration (Planned M4)
- **Purpose:** Validate deformation pipeline
- **Test:** Apply skinning, verify mesh vertices transformed
- **Modules:** animation, geometry
