# Render Graph Exports

Graphviz DOT snapshots capture representative planner outputs for documentation and telemetry reviews.

## Available Snapshots

| Profile | File | Description |
| --- | --- | --- |
| Deferred PBR baseline | [`deferred_pbr.dot`](./deferred_pbr.dot) | Frame graph assembled from `render.gbuffer`, `render.lighting`, and `render.present` nodes scheduled on the graphics queue. |

## Regeneration

Use `FrameGraphPlanner::Plan::to_dot()` to export fresh graphs after modifying planner logic or adding new nodes.

```cpp
FrameGraphPlanner planner{registry};
auto plan = planner.plan(request);
auto dot = plan.to_dot();
```

Run the existing rendering planner tests (`engine_rendering_tests`) to confirm DOT exports remain stable before updating the snapshots.
