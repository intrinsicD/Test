# Scene Hierarchy Diagnostics Samples

This directory hosts runnable fixtures for the hierarchy diagnostics workflow
(`SC-225`). The `scene_hierarchy_diagnostics_sample` binary can load existing
`.scene` files, emit JSON summaries, and regenerate the fixtures when they need
updates.

## Usage

```bash
cmake --build --preset <preset> --target scene_hierarchy_diagnostics_sample
$ cmake --build --preset linux-gcc-debug --target scene_hierarchy_diagnostics_sample
./out/build/<preset>/engine/scene/scene_hierarchy_diagnostics_sample --scene \
    engine/scene/samples/data/invalid_hierarchy.scene --pretty
```

Pass `--fail-on-issues` to return a non-zero exit code when validation fails and
`--pretty` for indented JSON. Use `--sample <name>` to evaluate the built-in
fixtures in memory or `--list-samples` to enumerate the available names.

## Regenerating Fixtures

Regenerate the committed `.scene` files when the sample definitions change:

```bash
./out/build/<preset>/engine/scene/scene_hierarchy_diagnostics_sample \
    --emit-samples engine/scene/samples/data
```

The command overwrites existing fixtures with deterministically serialised
versions produced by the engine serializer.
