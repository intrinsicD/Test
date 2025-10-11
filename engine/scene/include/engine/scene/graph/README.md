# Engine Scene Graph

## Current State

- Placeholder for higher-level traversal helpers layered on top of the raw EnTT registry views.
- Planned home for iterators that expose hierarchical orderings (pre/post-order) and dependency queries for system scheduling.

## Usage

- Keep this directory aligned with its parent module and update the README as features land.

## TODO / Next Steps

- Formalise traversal primitives (depth-first iterators, ancestor/descendant queries, sibling ranges).
- Describe dependency evaluation rules that systems can consume to reason about update ordering.
- Provide benchmarks and documentation once the initial traversal utilities are implemented.
