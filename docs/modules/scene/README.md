# Scene Module

## Overview

The scene module provides scene graph management with entity-component patterns, hierarchical transforms with propagation, deterministic serialization, and comprehensive validation including cycle detection (`RT-005`). It integrates with the core ECS registry and runtime diagnostics.

## Entity Management

### Scene API

Create and manage scenes with entities:

```cpp
#include "engine/scene/api.hpp"

scene::Scene my_scene;

// Create entity
auto entity = my_scene.create_entity("player");

// Add components
my_scene.add_component<scene::Transform>(entity, {
    .translation = {0.0f, 1.0f, 0.0f},
    .rotation = math::quat::identity(),
    .scale = {1.0f, 1.0f, 1.0f}
});

my_scene.add_component<scene::Name>(entity, "PlayerCharacter");

// Query entities
auto view = my_scene.view<scene::Transform, scene::Name>();
for (auto [entity, transform, name] : view.each()) {
    fmt::print("Entity: {}, Position: ({}, {}, {})\n",
        name.value, 
        transform.translation.x,
        transform.translation.y,
        transform.translation.z);
}
```

### Built-in Components

Common components provided by the scene module:

```cpp
// Transform component
struct Transform {
    math::vec3 translation{0.0f, 0.0f, 0.0f};
    math::quat rotation{1.0f, 0.0f, 0.0f, 0.0f};
    math::vec3 scale{1.0f, 1.0f, 1.0f};
    
    math::mat4 to_matrix() const;
};

// Name component
struct Name {
    std::string value;
};

// Hierarchy component
struct Hierarchy {
    entt::entity parent{entt::null};
    std::vector<entt::entity> children;
};
```

## Hierarchical Transforms

### Parent-Child Relationships

Build scene hierarchies:

```cpp
auto parent = my_scene.create_entity("root");
auto child1 = my_scene.create_entity("child_1");
auto child2 = my_scene.create_entity("child_2");

// Establish hierarchy
my_scene.set_parent(child1, parent);
my_scene.set_parent(child2, parent);

// Local transforms
my_scene.get_component<scene::Transform>(child1).translation = {1.0f, 0.0f, 0.0f};

// World transform computed automatically
math::mat4 world_transform = my_scene.get_world_transform(child1);
```

### Transform Propagation

The transform system propagates changes through the hierarchy:

```cpp
// Update parent transform
auto& parent_transform = my_scene.get_component<scene::Transform>(parent);
parent_transform.translation = {0.0f, 5.0f, 0.0f};

// Child world transforms updated automatically on next update
my_scene.update_transforms();
```

Transform propagation is deterministic and processes entities in breadth-first order.

## Hierarchy Validation (`RT-005`)

### Cycle Detection

The scene module automatically detects cycles in parent-child relationships:

```cpp
auto result = scene::validation::validate_hierarchy(my_scene);

if (result.has_cycles) {
    fmt::print("Detected {} cycles:\n", result.cycle_count);
    
    for (const auto& cycle : result.cycles) {
        fmt::print("  Cycle: ");
        for (auto entity : cycle.entities) {
            fmt::print("{} -> ", my_scene.get_name(entity));
        }
        fmt::print("...\n");
    }
}
```

### Depth Analysis

Track hierarchy depth for performance analysis:

```cpp
if (result.max_depth > 100) {
    fmt::print("Warning: Deep hierarchy detected (max depth: {})\n", 
        result.max_depth);
}

// Per-entity depth
for (const auto& [entity, depth] : result.entity_depths) {
    if (depth > warning_threshold) {
        fmt::print("Entity {} at depth {}\n", 
            my_scene.get_name(entity), depth);
    }
}
```

### Orphan Detection

Find entities with invalid parent references:

```cpp
if (!result.orphan_entities.empty()) {
    fmt::print("Found {} orphaned entities:\n", result.orphan_entities.size());
    for (auto entity : result.orphan_entities) {
        fmt::print("  {}\n", my_scene.get_name(entity));
    }
}
```

### Validation Integration with Runtime

Runtime automatically validates scenes during tick:

```cpp
const auto& diag = runtime::diagnostics();
const auto& scene_validation = diag.scene_validation;

if (scene_validation.has_cycles) {
    fmt::print("Scene has cycles! Alert level: {}\n",
        static_cast<int>(diag.scene_validation_alert_level));
}

// Consecutive failure tracking
fmt::print("Consecutive failures: {}/{} frames\n",
    diag.scene_validation_consecutive_failure_frames,
    diag.scene_validation_max_consecutive_failure_frames);
```

Alert levels (from `SC-230`):
- `None`: No validation issues
- `Warning`: Minor issues, scene still functional
- `Critical`: Cycles or orphans detected, requires intervention

See [`DIAGNOSTICS.md`](DIAGNOSTICS.md) for complete validation workflow.

## Serialization

### Save Scene

```cpp
#include "engine/scene/serialization/serializer.hpp"

scene::Serializer serializer;

// Serialize to JSON
std::ofstream out("scene.json");
auto result = serializer.serialize(my_scene, out);

if (!result) {
    fmt::print("Serialization failed: {}\n", result.error().message);
}
```

### Load Scene

```cpp
std::ifstream in("scene.json");
auto result = serializer.deserialize(in);

if (result) {
    scene::Scene loaded_scene = std::move(*result);
} else {
    fmt::print("Deserialization failed: {}\n", result.error().message);
}
```

Serialization is deterministic: serializing and deserializing produces identical scenes, suitable for save/load systems and networked multiplayer.

## Systems

### Transform System

The transform system updates world transforms:

```cpp
#include "engine/scene/systems/transform_system.hpp"

scene::systems::TransformSystem transform_system;

// Update all transforms
transform_system.update(my_scene);
```

Called automatically by the runtime during tick.

### Hierarchy System

The hierarchy system maintains parent-child relationships:

```cpp
#include "engine/scene/systems/hierarchy_system.hpp"

scene::systems::HierarchySystem hierarchy_system;

// Update hierarchy bookkeeping
hierarchy_system.update(my_scene);
```

## Samples

The module includes sample scenes:

```cpp
// Create demo scene with hierarchy
auto demo_scene = scene::samples::create_demo_hierarchy();

// Stress test scene (deep hierarchy)
auto stress_scene = scene::samples::create_stress_test_scene(
    /*depth=*/50, 
    /*children_per_node=*/3
);
```

## Integration with Runtime

The runtime exposes scene state:

```cpp
const auto& runtime_state = runtime_host.tick(dt);

for (const auto& node : runtime_state.scene_nodes) {
    fmt::print("Node: {}\n", node.name);
    fmt::print("  Translation: ({}, {}, {})\n",
        node.transform.translation.x,
        node.transform.translation.y,
        node.transform.translation.z);
}
```

## Performance Considerations

Transform propagation complexity:
- Best case: O(n) for flat hierarchy
- Worst case: O(n) for tree hierarchy
- Average: ~0.5ms for 1000 entities with depth 10

Validation overhead:
- Cycle detection: O(n) per frame
- Depth analysis: O(n) per frame
- Total: ~0.2ms for 1000 entities

Optimization tips:
- Keep hierarchy depth below 20 for optimal performance
- Batch transform updates when possible
- Use validation alert levels to avoid redundant checks

## Testing

Scene tests validate:
- Entity lifecycle (`test_scene.cpp`)
- Hierarchy management (`test_hierarchy.cpp`)
- Transform propagation (`test_transform_system.cpp`)
- Cycle detection (`test_validation.cpp`)
- Serialization roundtrip (`test_serialization.cpp`)

Run tests:
```bash
ctest --preset linux-gcc-debug -R scene
```

Sample scenes for debugging:
```bash
ctest --preset linux-gcc-debug -R scene_samples
```

## Dependencies

- **Core**: ECS registry (EnTT wrapper), telemetry schema
- **Math**: Transform types, matrix operations
- **Runtime** (integration): Validation diagnostics, lifecycle orchestration

## Related Documentation

- [`BACKLOG.md`](BACKLOG.md): Module milestones including `RT-005` validation work
- [`DIAGNOSTICS.md`](DIAGNOSTICS.md): Scene validation diagnostics reference
- [`../../ARCHITECTURE.md`](../../ARCHITECTURE.md): Scene role in data flow
- [`../../tasks/2025-02-17-sprint-06.md`](../../tasks/2025-02-17-sprint-06.md): Scene validation sprint milestone

## Current State

- Entity façade, hierarchy and transform propagation, deterministic serialization, and validation with cycle detection; runtime diagnostics integration exposes hierarchy health and alert levels.

## Usage

- Run scene tests:
  - `ctest --preset linux-gcc-debug -R scene`
- See `engine/scene/tests/` for hierarchy, serialization, and validation coverage.

## TODO / Next Steps

- Keep the hierarchy diagnostics samples and dashboards aligned (`SC-225`), and maintain alert thresholds policy (`SC-230`); see ../../ROADMAP.md
- Expand remediation guides with concrete fixtures once samples land; see ../../ROADMAP.md
