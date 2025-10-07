# Scene

_Path: `engine/scene`_

_Last updated: 2025-10-05_


## Contents

### Subdirectories

- `components/` – documented in its own README; contains 1 file.
- `graph/` – documented in its own README; contains 1 file.
- `include/` – contains 1 subdirectory.
- `serialization/` – documented in its own README; contains 1 file.
- `src/` – documented in its own README; contains 1 file.
- `systems/` – documented in its own README; contains 1 file.
- `tests/` – documented in its own README; contains 2 files.

### Files

- `CMakeLists.txt` – Text resource.

## Public headers

The scene module exposes both its root directory and the `include/` tree as
public include directories. This allows client code to include implementation
details such as

```cpp
#include "components/hierarchy.hpp"
#include "systems/registry.hpp"
```

without relying on fragile, relative paths.
