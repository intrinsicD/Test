# [Module Name]

## Purpose
[One sentence describing what this module solves for the engine.]

## Current State
- [What is implemented and stable?]
- [What is experimental or in development?]

## Dependencies

### Required
- `engine::math` – [Justification]
- `engine::platform` – [Justification]

### Optional
- `engine::physics` – [Which features consume this?]

### Exports
- `engine::[module]` – Primary CMake target.
- [List additional libraries or header-only exports.]

## Public API Stability

- **Stable APIs**: [Namespaces/classes considered stable.]
- **Experimental APIs**: [Features under active development with expected stabilisation window.]
- **Deprecated APIs**: [Items slated for removal and their planned timeline.]

## Usage Example

```cpp
#include "engine/[module]/api.hpp"

// Minimal example showing how a client integrates the module.
```

## Integration Points

### For Physics Developers
[Describe how this module collaborates with physics.]

### For Rendering Developers
[Describe how this module collaborates with rendering.]

## Build Configuration

```cmake
# Minimum integration
 target_link_libraries(my_target
     PRIVATE engine::[module]
 )

# Optional dependencies or feature toggles
 target_link_libraries(my_target
     PRIVATE engine::[module]
     PRIVATE engine::[optional_dep]
 )
 target_compile_definitions(my_target
     PRIVATE ENGINE_[MODULE]_ENABLE_[FEATURE]
 )
```

## Performance Characteristics
- **Memory usage**: [Typical allocation patterns, pools used.]
- **Thread safety**: [Reentrant? Requires external synchronisation?]
- **Initialisation cost**: [Startup considerations.]

## Testing

```bash
ctest -R engine_[module]_tests

# With sanitizers enabled
cmake -DENABLE_SANITIZERS=ON ..
ctest -R engine_[module]_tests
```

## TODO / Next Steps

### Version 1.0 (Stable Release)
- [ ] [Task with owner] – Due: [Date]
- [ ] [Task with owner] – Due: [Date]

### Version 1.1 (Next Release)
- [ ] [Feature request with implementation estimate]

### Future Considerations
- [Longer-term ideas without immediate priority.]

## Contributing
See `../CONTRIBUTING.md` for project-wide guidance. Document any module-specific requirements here.
