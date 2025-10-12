# Rendering Module Roadmap

## Near Term
- Expand frame graph resource descriptions with queue and barrier metadata, and extend tests to validate hazard tracking.
- Prototype a reference GPU scheduler capable of translating frame graph tasks into backend submissions, initially targeting Vulkan.

## Mid Term
- Implement backend adapters for Vulkan and Direct3D12 including resource lifetime tracking and descriptor heap management.
- Build a library of reusable render passes (shadow maps, SSAO, tone mapping) with sample scenes demonstrating integration.

## Long Term
- Integrate runtime telemetry (GPU timing, resource residency) and author documentation/samples to support production rendering workloads.
