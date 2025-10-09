from __future__ import annotations

from pathlib import Path
import textwrap

ROOT = Path('.')

TODO_TEXT = {
    'module_engine': "Stabilise cross-module integration and build targets for contributors.",
    'module_animation': "Add blend-tree authoring and clip serialization to unlock complex rigs.",
    'module_assets': "Design asset pipelines that feed runtime modules beyond sample data.",
    'module_compute': "Implement GPU-backed dispatch and integrate with runtime scheduling.",
    'module_core': "Extend engine core services for application control, configuration, and diagnostics.",
    'module_geometry': "Introduce advanced mesh processing (remeshing, parameterization, collision prep).",
    'module_io': "Wire format import/export and caching to serve real content pipelines.",
    'module_math': "Broaden math primitives and numerics beyond the current minimal set.",
    'module_physics': "Add collision detection, constraints, and stable integration schemes.",
    'module_platform': "Bind platform abstractions to OS windowing, input, and filesystem APIs.",
    'module_rendering': "Connect the frame graph to concrete GPU backends and resource providers.",
    'module_runtime': "Drive subsystem lifecycles and state management for end-to-end scenarios.",
    'module_scene': "Implement scene graph serialization, systems, and runtime traversal.",
    'module_tests': "Curate representative scenarios across unit, integration, and performance suites.",
    'module_tools': "Stand up tooling flows for content pipelines, profiling, and editor workflows.",
    'docs_root': "Expand living design documents with current architectural decisions.",
    'docs_api': "Document subsystem APIs and keep diagrams in sync with code changes.",
    'docs_design': "Capture design rationales and outcomes for upcoming milestones.",
    'python_root': "Publish the Python packaging and dependency manifest for automation helpers.",
    'python_engine3g': "Harden the loader API and expose ergonomic runtime bindings.",
    'scripts_root': "Consolidate automation entry points for developers and CI environments.",
    'scripts_build': "Add platform-aware build presets and dependency bootstrapping scripts.",
    'scripts_ci': "Implement CI harness scripts mirroring the documented workflow.",
    'include_docs': "Document the public headers once the API stabilises.",
    'tests_coverage': "Expand regression coverage beyond the current smoke checks.",
    'src_examples': "Add scenario-driven examples and profiling to exercise the implementation.",
    'placeholder_fill': "Populate this placeholder directory with its intended implementation when scoped.",
    'backend_integration': "Implement the actual backend integration using the target API.",
    'shader_assets': "Author representative shader assets once the rendering pipeline is defined.",
    'resources_pipeline': "Provide resource pipeline integration for GPU buffer and sampler management.",
    'passes_pipeline': "Design render passes and pipelines once a resource provider is available.",
    'visibility_system': "Implement visibility determination algorithms to drive culling.",
    'materials_system': "Hook materials into the rendering backend and asset pipeline.",
    'scene_components': "Define component schemas and authoring utilities for the runtime.",
    'scene_systems': "Implement scene systems that operate over ECS data during the frame.",
    'scene_serialization': "Implement scene serialization and deserialization compatible with runtime assets.",
    'scene_graph': "Build scene graph traversal and dependency evaluation.",
    'animation_rigging': "Prototype deformation and rigging utilities for the animation pipeline.",
    'compute_cuda': "Integrate CUDA kernels and scheduling into the dispatcher.",
    'io_cache': "Implement persistent cache strategies for imported assets.",
    'io_importers': "Add format importers to translate real data into runtime structures.",
    'io_exporters': "Create exporters to persist runtime assets back to disk.",
    'assets_samples': "Provide curated sample assets demonstrating runtime features.",
    'platform_windowing': "Replace the stub window factories with real OS backends.",
    'platform_input': "Implement actual input device polling per platform.",
    'platform_filesystem': "Integrate local and virtual filesystem providers for asset loading.",
    'tools_editor': "Develop the editor shell and hook it into the runtime subsystems.",
    'tools_pipelines': "Build content pipeline automation that feeds assets into the engine.",
    'tools_profiling': "Instrument and visualise profiling data for frame and job execution.",
    'rendering_lighting': "Implement lighting models and data flows for the renderer.",
    'rendering_pipeline': "Connect the frame graph to GPU submission and synchronization.",
    'rendering_resources': "Implement GPU resource management for buffers, textures, and samplers.",
}

SPECIAL_TODOS = {
    'engine/animation/deformation/README.md': 'animation_rigging',
    'engine/animation/rigging/README.md': 'animation_rigging',
    'engine/animation/skinning/README.md': 'animation_rigging',
    'engine/animation/src/README.md': 'src_examples',
    'engine/animation/tests/README.md': 'tests_coverage',
    'engine/animation/include/engine/animation/README.md': 'include_docs',

    'engine/assets/src/README.md': 'src_examples',
    'engine/assets/tests/README.md': 'tests_coverage',
    'engine/assets/include/engine/assets/README.md': 'include_docs',
    'engine/assets/samples/README.md': 'assets_samples',
    'engine/assets/shaders/README.md': 'shader_assets',

    'engine/compute/src/README.md': 'src_examples',
    'engine/compute/tests/README.md': 'tests_coverage',
    'engine/compute/include/engine/compute/README.md': 'include_docs',
    'engine/compute/cuda/README.md': 'compute_cuda',
    'engine/compute/cuda/src/README.md': 'compute_cuda',
    'engine/compute/cuda/tests/README.md': 'tests_coverage',
    'engine/compute/cuda/include/engine/compute/cuda/README.md': 'include_docs',

    'engine/core/src/README.md': 'src_examples',
    'engine/core/tests/README.md': 'tests_coverage',
    'engine/core/include/engine/core/README.md': 'include_docs',
    'engine/core/application/README.md': 'placeholder_fill',
    'engine/core/configuration/README.md': 'placeholder_fill',
    'engine/core/diagnostics/README.md': 'placeholder_fill',
    'engine/core/ecs/README.md': 'src_examples',
    'engine/core/memory/README.md': 'placeholder_fill',
    'engine/core/parallel/README.md': 'placeholder_fill',
    'engine/core/plugin/README.md': 'placeholder_fill',
    'engine/core/runtime/README.md': 'placeholder_fill',

    'engine/geometry/src/README.md': 'src_examples',
    'engine/geometry/tests/README.md': 'tests_coverage',
    'engine/geometry/include/engine/geometry/README.md': 'include_docs',
    'engine/geometry/include/engine/geometry/shapes/README.md': 'include_docs',
    'engine/geometry/csg/README.md': 'placeholder_fill',
    'engine/geometry/decimation/README.md': 'placeholder_fill',
    'engine/geometry/mesh/README.md': 'placeholder_fill',
    'engine/geometry/src/shapes/README.md': 'src_examples',
    'engine/geometry/surfaces/README.md': 'placeholder_fill',
    'engine/geometry/topology/README.md': 'placeholder_fill',
    'engine/geometry/uv/README.md': 'placeholder_fill',
    'engine/geometry/volumetric/README.md': 'placeholder_fill',

    'engine/io/include/engine/io/README.md': 'include_docs',
    'engine/io/src/README.md': 'src_examples',
    'engine/io/tests/README.md': 'tests_coverage',
    'engine/io/cache/README.md': 'io_cache',
    'engine/io/exporters/README.md': 'io_exporters',
    'engine/io/importers/README.md': 'io_importers',

    'engine/math/include/engine/math/README.md': 'include_docs',
    'engine/math/tests/README.md': 'tests_coverage',
    'engine/math/README.md': 'module_math',

    'engine/physics/include/engine/physics/README.md': 'include_docs',
    'engine/physics/src/README.md': 'src_examples',
    'engine/physics/tests/README.md': 'tests_coverage',
    'engine/physics/collision/README.md': 'placeholder_fill',
    'engine/physics/dynamics/README.md': 'placeholder_fill',

    'engine/platform/include/engine/platform/README.md': 'include_docs',
    'engine/platform/src/README.md': 'src_examples',
    'engine/platform/tests/README.md': 'tests_coverage',
    'engine/platform/windowing/README.md': 'platform_windowing',
    'engine/platform/input/README.md': 'platform_input',
    'engine/platform/filesystem/README.md': 'platform_filesystem',

    'engine/rendering/include/engine/rendering/README.md': 'include_docs',
    'engine/rendering/src/README.md': 'src_examples',
    'engine/rendering/tests/README.md': 'tests_coverage',
    'engine/rendering/backend/README.md': 'backend_integration',
    'engine/rendering/backend/opengl/README.md': 'backend_integration',
    'engine/rendering/backend/vulkan/README.md': 'backend_integration',
    'engine/rendering/backend/metal/README.md': 'backend_integration',
    'engine/rendering/backend/directx12/README.md': 'backend_integration',
    'engine/rendering/resources/README.md': 'rendering_resources',
    'engine/rendering/resources/buffers/README.md': 'rendering_resources',
    'engine/rendering/resources/samplers/README.md': 'rendering_resources',
    'engine/rendering/passes/README.md': 'passes_pipeline',
    'engine/rendering/pipeline/README.md': 'rendering_pipeline',
    'engine/rendering/lighting/README.md': 'rendering_lighting',
    'engine/rendering/materials/README.md': 'materials_system',
    'engine/rendering/materials/shaders/README.md': 'shader_assets',
    'engine/rendering/materials/shaders/common/README.md': 'shader_assets',
    'engine/rendering/materials/shaders/glsl/README.md': 'shader_assets',
    'engine/rendering/materials/shaders/hlsl/README.md': 'shader_assets',
    'engine/rendering/materials/textures/README.md': 'placeholder_fill',
    'engine/rendering/visibility/README.md': 'visibility_system',

    'engine/runtime/include/engine/runtime/README.md': 'include_docs',
    'engine/runtime/src/README.md': 'src_examples',
    'engine/runtime/tests/README.md': 'tests_coverage',

    'engine/scene/include/engine/scene/README.md': 'include_docs',
    'engine/scene/src/README.md': 'src_examples',
    'engine/scene/tests/README.md': 'tests_coverage',
    'engine/scene/components/README.md': 'scene_components',
    'engine/scene/graph/README.md': 'scene_graph',
    'engine/scene/systems/README.md': 'scene_systems',
    'engine/scene/serialization/README.md': 'scene_serialization',

    'engine/tools/editor/README.md': 'tools_editor',
    'engine/tools/pipelines/README.md': 'tools_pipelines',
    'engine/tools/profiling/README.md': 'tools_profiling',

    'python/engine3g/README.md': 'python_engine3g',

    'scripts/build/README.md': 'scripts_build',
    'scripts/ci/README.md': 'scripts_ci',
}

MANUAL_CONTENT = {
    'docs/README.md': {
        'title': 'Documentation Hub',
        'current': [
            '- Central index that links design notes, API references, and validation utilities.',
            '- Serves as the landing page for contributors seeking written guidance about the engine.',
        ],
        'usage': [
            '- Use this entry point to navigate to API and design subdirectories before changing the code.',
            '- Reference `docs/README_TEMPLATE.md` when drafting or updating module READMEs.',
            '- Run `python scripts/validate_docs.py` to keep cross-references intact after edits.',
        ],
        'todo': 'docs_root',
    },
    'docs/api/README.md': {
        'title': 'API Documentation',
        'current': [
            '- Collects subsystem API summaries and call sequences for the runtime and tooling layers.',
            '- Provides per-module Markdown files that mirror the exported headers.',
        ],
        'usage': [
            '- Update the relevant API note whenever a public header changes.',
            '- Keep diagrams or tables close to the code they describe to minimise drift.',
        ],
        'todo': 'docs_api',
    },
    'docs/design/README.md': {
        'title': 'Design Records',
        'current': [
            '- Houses architecture explorations, design rationales, and decision records for the engine.',
            '- Tracks historical context for feature planning and subsystem experiments.',
        ],
        'usage': [
            '- Capture design proposals here before landing major subsystem work.',
            '- Reference these documents in pull requests so reviewers can trace the intent.',
        ],
        'todo': 'docs_design',
    },
    'engine/README.md': {
        'title': 'Engine Modules',
        'current': [
            '- Aggregates all native subsystems including animation, rendering, physics, geometry, and platform glue.',
            '- Each subdirectory builds as an independent static/shared library that contributes to the runtime.',
        ],
        'usage': [
            '- Add new subsystems as sibling directories with their own CMake targets and README following the template.',
            '- Include this directory from the root `CMakeLists.txt` to expose all engine libraries.',
        ],
        'todo': 'module_engine',
    },
    'engine/animation/README.md': {
        'title': 'Animation Module',
        'current': [
            '- Provides clip, track, and controller primitives with sampling implemented in `src/api.cpp`.',
            '- Supplies a deterministic oscillator clip that powers runtime smoke tests and demonstrations.',
        ],
        'usage': [
            '- Link against `engine_animation` and include `<engine/animation/api.hpp>` to access the API.',
            '- Extend controllers or clips under `src/` and update accompanying tests in `tests/`.',
        ],
        'todo': 'module_animation',
    },
    'engine/assets/README.md': {
        'title': 'Asset Module',
        'current': [
            '- Defines the staging area for runtime asset management across samples, shaders, and packaging.',
            '- Currently focuses on directory scaffolding; concrete asset loaders remain to be implemented.',
        ],
        'usage': [
            '- Organise engine-owned assets here so runtime modules can locate them consistently.',
            '- Keep asset metadata in sync with runtime expectations while pipelines mature.',
        ],
        'todo': 'module_assets',
    },
    'engine/compute/README.md': {
        'title': 'Compute Module',
        'current': [
            '- Hosts a CPU kernel dispatcher capable of enforcing dependency order and emitting execution traces.',
            '- Exposes a minimal math helper (`identity_transform`) shared across GPU and CPU code paths.',
        ],
        'usage': [
            '- Link against `engine_compute` to enqueue kernels and dispatch them via the topological scheduler.',
            '- Extend the dispatcher or integrate GPU execution paths under the CUDA submodule.',
        ],
        'todo': 'module_compute',
    },
    'engine/core/README.md': {
        'title': 'Core Module',
        'current': [
            '- Implements foundational services such as the entity registry wrapper and module naming API.',
            '- Provides scaffolding for application lifecycle, configuration, diagnostics, and plugin subsystems.',
        ],
        'usage': [
            '- Include `<engine/core/api.hpp>` and link against `engine_core` to use the base facilities.',
            '- Extend ECS features or add new core services in the dedicated subdirectories.',
        ],
        'todo': 'module_core',
    },
    'engine/geometry/README.md': {
        'title': 'Geometry Module',
        'current': [
            '- Supplies a `SurfaceMesh` abstraction with helpers for bounds updates, centroid evaluation, and normals.',
            '- Provides procedural generation for a unit quad to seed rendering and physics smoke tests.',
        ],
        'usage': [
            '- Link against `engine_geometry` and include `<engine/geometry/api.hpp>` to manipulate meshes.',
            '- Extend mesh processing algorithms under `src/` and pair them with tests in `tests/`.',
        ],
        'todo': 'module_geometry',
    },
    'engine/io/README.md': {
        'title': 'I/O Module',
        'current': [
            '- Establishes the directory layout for importers, exporters, caching, and runtime I/O infrastructure.',
            '- Provides initial stubs that will host asset streaming and persistence logic.',
        ],
        'usage': [
            '- Implement specific format handlers under `importers/` and `exporters/` as pipelines solidify.',
            '- Keep cache policies and runtime integration code in `src/` aligned with subsystem expectations.',
        ],
        'todo': 'module_io',
    },
    'engine/math/README.md': {
        'title': 'Math Module',
        'current': [
            '- Wraps vector, matrix, and quaternion helpers that support animation, geometry, and physics subsystems.',
            '- Acts as the shared numerical foundation for both native and Python bindings.',
        ],
        'usage': [
            '- Include `<engine/math/math.hpp>` from consumers to access the linear algebra primitives.',
            '- Extend the math toolkit in tandem with new subsystem requirements and corresponding tests.',
        ],
        'todo': 'module_math',
    },
    'engine/physics/README.md': {
        'title': 'Physics Module',
        'current': [
            '- Offers a simple rigid-body world with force accumulation, Euler integration, and mass management.',
            '- Exposes safe accessors that validate indices and ensure deterministic updates.',
        ],
        'usage': [
            '- Link against `engine_physics` and include `<engine/physics/api.hpp>` to simulate bodies.',
            '- Extend dynamics, collision, and constraints logic within `src/` and cover it via tests.',
        ],
        'todo': 'module_physics',
    },
    'engine/platform/README.md': {
        'title': 'Platform Module',
        'current': [
            '- Defines abstractions for filesystem access, input, windowing, and platform-specific utilities.',
            '- Currently stubs out real OS integration pending backend work.',
        ],
        'usage': [
            '- Link against `engine_platform` when consuming platform services from other modules.',
            '- Implement platform-specific backends in the dedicated subdirectories as requirements mature.',
        ],
        'todo': 'module_platform',
    },
    'engine/rendering/README.md': {
        'title': 'Rendering Module',
        'current': [
            '- Implements a prototype frame graph and forward pipeline within the native renderer.',
            '- Structures backend, resource, material, and pass directories for platform-specific integrations.',
        ],
        'usage': [
            '- Link against `engine_rendering` to access render pass orchestration and frame graph primitives.',
            '- Add backend-specific code under `backend/` and keep resource and material definitions in sync.',
        ],
        'todo': 'module_rendering',
    },
    'engine/runtime/README.md': {
        'title': 'Runtime Module',
        'current': [
            '- Provides the aggregation layer that will stitch subsystems into an executable runtime.',
            '- Includes scaffolding for lifecycle management and module loading.',
        ],
        'usage': [
            '- Link against `engine_runtime` to bootstrap the engine once subsystems are wired together.',
            '- Expand runtime orchestration logic under `src/` and keep tests aligned with subsystem growth.',
        ],
        'todo': 'module_runtime',
    },
    'engine/scene/README.md': {
        'title': 'Scene Module',
        'current': [
            '- Outlines the scene graph, component storage, serialization, and system execution layout.',
            '- Currently focuses on structure and scaffolding pending concrete implementations.',
        ],
        'usage': [
            '- Link against `engine_scene` to host world state once components and systems are implemented.',
            '- Keep serialization and graph updates coordinated with runtime requirements.',
        ],
        'todo': 'module_scene',
    },
    'engine/tests/README.md': {
        'title': 'Engine Test Suite',
        'current': [
            '- Collects unit, integration, and performance suites for all native subsystems.',
            '- Integrates with CTest via the top-level CMake configuration.',
        ],
        'usage': [
            '- Configure the project with `-DBUILD_TESTING=ON` and execute `ctest --test-dir build`.',
            '- Add scenario coverage under the appropriate subdirectory as new features land.',
        ],
        'todo': 'module_tests',
    },
    'engine/tools/README.md': {
        'title': 'Tools Module',
        'current': [
            '- Houses editor, pipeline automation, and profiling utilities that sit atop the runtime.',
            '- Presently acts as scaffolding for future tooling features.',
        ],
        'usage': [
            '- Extend tooling subdirectories with standalone applications or scripts as capabilities evolve.',
            '- Keep tooling build targets optional to avoid bloating the default configuration.',
        ],
        'todo': 'module_tools',
    },
    'python/README.md': {
        'title': 'Python Tooling',
        'current': [
            '- Provides the root for Python-based automation, testing, and runtime bindings.',
            '- Relies on the native modules to be discoverable via environment configuration.',
        ],
        'usage': [
            '- Create and activate a virtual environment, then install dependencies once `requirements.txt` is defined.',
            '- Execute `pytest` after building native modules to validate the Python side.',
        ],
        'todo': 'python_root',
    },
    'python/engine3g/README.md': {
        'title': 'Python Engine Bindings',
        'current': [
            '- Contains the Python loader that discovers and interacts with compiled engine modules.',
            '- Establishes the namespace for higher-level scripting utilities.',
        ],
        'usage': [
            '- Ensure `ENGINE3G_LIBRARY_PATH` points to the built shared libraries before importing this package.',
            '- Add ergonomic wrappers or CLI entry points alongside new runtime capabilities.',
        ],
        'todo': 'python_engine3g',
    },
    'scripts/README.md': {
        'title': 'Automation Scripts',
        'current': [
            '- Provides entry points for local developer workflows and CI orchestration.',
            '- Currently outlines the script categories without concrete implementations in place.',
        ],
        'usage': [
            '- Run scripts from the repository root to maintain consistent relative paths.',
            '- Reflect any workflow changes across CI and local scripts to avoid divergence.',
        ],
        'todo': 'scripts_root',
    },
    'scripts/build/README.md': {
        'title': 'Build Scripts',
        'current': [
            '- Placeholder for build automation helpers such as preset generators or dependency bootstrappers.',
            '- Directory currently lacks concrete implementation scripts.',
        ],
        'usage': [
            '- Add platform-aware build entry points here as the project scales.',
            '- Keep build scripts aligned with documented setup instructions.',
        ],
        'todo': 'scripts_build',
    },
    'scripts/ci/README.md': {
        'title': 'CI Scripts',
        'current': [
            '- Reserved for continuous integration helpers mirroring the documented workflow.',
            '- Currently empty aside from the README scaffold.',
        ],
        'usage': [
            '- Populate with pipeline definitions or helper scripts used by CI providers.',
            '- Ensure local and CI workflows remain consistent to prevent surprises.',
        ],
        'todo': 'scripts_ci',
    },
}

WORD_MAP = {
    'src': 'Sources',
    'include': 'Public Headers',
    'tests': 'Tests',
    'backend': 'Backend',
    'opengl': 'OpenGL',
    'vulkan': 'Vulkan',
    'metal': 'Metal',
    'directx12': 'DirectX 12',
    'shaders': 'Shader Assets',
    'common': 'Common',
    'glsl': 'GLSL',
    'hlsl': 'HLSL',
    'materials': 'Materials',
    'textures': 'Textures',
    'resources': 'Resources',
    'buffers': 'Buffers',
    'samplers': 'Samplers',
    'passes': 'Render Passes',
    'pipeline': 'Pipeline',
    'lighting': 'Lighting',
    'visibility': 'Visibility',
    'components': 'Components',
    'systems': 'Systems',
    'serialization': 'Serialization',
    'graph': 'Graph',
    'samples': 'Samples',
    'shaders': 'Shader Assets',
    'cache': 'Cache',
    'importers': 'Importers',
    'exporters': 'Exporters',
    'filesystem': 'Filesystem',
    'windowing': 'Windowing',
    'input': 'Input',
    'application': 'Application',
    'configuration': 'Configuration',
    'diagnostics': 'Diagnostics',
    'memory': 'Memory',
    'parallel': 'Parallel',
    'plugin': 'Plugin',
    'runtime': 'Runtime',
    'ecs': 'ECS',
    'deformation': 'Deformation',
    'rigging': 'Rigging',
    'skinning': 'Skinning',
    'cuda': 'CUDA',
    'pipelines': 'Pipelines',
    'profiling': 'Profiling',
    'editor': 'Editor',
}

DEFAULT_TODO_KEYS = {
    'include': 'include_docs',
    'tests': 'tests_coverage',
    'src': 'src_examples',
}

def friendly_segment(segment: str) -> str:
    return WORD_MAP.get(segment, segment.replace('_', ' ').title())


def friendly_title(path: Path) -> str:
    parts = [friendly_segment(part) for part in path.parts if part != 'README.md']
    if parts and parts[0] == '.':
        parts = parts[1:]
    if not parts:
        return 'Unknown Module'
    if parts[0] == 'Engine' and len(parts) > 1:
        parts[1] = parts[1].title()
    if 'include' in path.parts:
        module_segment = friendly_segment(path.parts[1])
        include_index = path.parts.index('include')
        tail_segments = [
            friendly_segment(segment)
            for segment in path.parts[include_index + 1 : -1]
        ]
        filtered_tail = [
            segment
            for segment in tail_segments
            if segment.lower() not in {module_segment.lower(), 'engine'}
        ]
        parts = ['Engine', module_segment, 'Public Headers', *filtered_tail]
    cleaned: list[str] = []
    for part in parts:
        if not part:
            continue
        if cleaned and cleaned[-1].lower() == part.lower():
            continue
        cleaned.append(part)
    return ' '.join(cleaned)


def summarize_files(path: Path) -> list[str]:
    dir_path = path.parent
    bullets: list[str] = []
    files = [p for p in dir_path.iterdir() if p.name != 'README.md']
    if not files:
        bullets.append('- Placeholder directory awaiting its initial implementation.')
        return bullets

    is_tests = 'tests' in path.parts

    suffix_map = {}
    for file in files:
        if file.is_file():
            suffix_map.setdefault(file.suffix, []).append(file)
        else:
            suffix_map.setdefault('<dir>', []).append(file)

    if is_tests:
        test_files = [file for file in files if file.suffix in {'.cpp', '.cc', '.cxx', '.py'}]
        if test_files:
            names = []
            for file in test_files:
                name = file.stem.replace('_', ' ').title()
                if name.startswith('Test '):
                    name = name[5:]
                names.append(name)
            names = sorted({name for name in names})
            joined = ', '.join(names)
            bullets.append(f'- Contains automated tests covering {joined}.')
        else:
            bullets.append('- Contains scaffolding for future automated tests.')
    elif any(file.suffix in {'.hpp', '.h', '.hh', '.hxx'} for file in files):
        bullets.append('- Exposes public headers that mirror the module API and guard ABI compatibility.')
    if not is_tests and any(file.suffix in {'.cpp', '.cc', '.cxx'} for file in files):
        names = []
        for file in files:
            if file.suffix in {'.cpp', '.cc', '.cxx'}:
                stem = file.stem.replace('_', ' ')
                names.append(stem)
        if names:
            unique = sorted({name.title() for name in names})
            joined = ', '.join(unique)
            bullets.append(f'- Implements the module logic across {joined}.')
    if any(file.suffix in {'.py'} for file in files):
        bullets.append('- Provides Python utilities that complement the engine toolchain.')
    if any(file.is_dir() for file in files):
        subdirs = ', '.join(sorted(friendly_segment(p.name) for p in files if p.is_dir()))
        bullets.append(f'- Hosts subdirectories for {subdirs}.')
    if not bullets:
        bullets.append('- Contains scaffolding files that will evolve alongside the subsystem.')
    return bullets


def usage_guidance(path: Path, dir_path: Path) -> list[str]:
    parts = path.parts
    if parts[0] == 'engine' and parts[1] == 'tests':
        if len(parts) == 3:
            return [
                '- Configure the project with `-DBUILD_TESTING=ON` and execute `ctest --test-dir build`.',
                '- Partition new suites into `unit/`, `integration/`, or `performance/` as coverage grows.',
            ]
        return [
            '- Enable testing in the build and run `ctest --test-dir build` to execute this suite.',
            '- Expand the scenarios here to cover the behaviours introduced by the parent module.',
        ]
    if parts[0] == 'engine':
        module = parts[1]
        if len(parts) == 3:  # module root
            return [
                f'- Enable the module by adding `add_subdirectory(engine/{module})` to the root CMake tree.',
                f'- Link against `engine_{module}` from consumer targets to access the subsystem.',
            ]
        if 'include' in parts:
            header_root = '/'.join(parts[parts.index('include') + 1:-1])
            return [
                f'- Include headers from `<{header_root}/...>` when consuming the public API.',
                '- Keep header changes paired with updates to the module tests and documentation.',
            ]
        if 'tests' in parts:
            return [
                '- Build the module with testing enabled and execute `ctest --test-dir build` to run this suite.',
                '- Extend the cases alongside new runtime features to maintain behavioural coverage.',
            ]
        if 'src' in parts:
            return [
                f'- Source files compile into `engine_{module}`; ensure the build target stays warning-clean.',
                '- Mirror API additions with implementation updates in this directory.',
            ]
    if parts[0] == 'docs':
        return [
            '- Keep these notes synchronized with the evolving codebase and architectural decisions.',
            '- Run `python scripts/validate_docs.py` after editing Markdown content.',
        ]
    if parts[0] == 'python':
        return [
            '- Activate the project virtual environment and install dependencies via `pip install -r requirements.txt`.',
            '- Run `pytest` from the repository root to exercise the Python helpers once native modules are built.',
        ]
    if parts[0] == 'scripts':
        return [
            '- Invoke the scripts from the repository root so relative paths resolve correctly.',
            '- Mirror any workflow changes in CI by updating the corresponding script entry points.',
        ]
    if parts[0] == 'engine' and parts[1] == 'tests':
        return [
            '- Configure the root project with `-DBUILD_TESTING=ON` and call `ctest --test-dir build`.',
            '- Add new suites or benchmarks under this hierarchy as subsystems mature.',
        ]
    return [
        '- Keep this directory aligned with its parent module and update the README as features land.',
    ]


def todo_key_for(path: Path) -> str:
    key = SPECIAL_TODOS.get(str(path))
    if key:
        return key
    parts = path.parts
    if parts[0] == 'engine':
        if len(parts) == 3:
            return f'module_{parts[1]}'
        if 'include' in parts:
            return 'include_docs'
        if 'tests' in parts:
            return 'tests_coverage'
        if 'src' in parts:
            return 'src_examples'
        return 'placeholder_fill'
    if parts[0] == 'docs':
        if len(parts) == 2:
            return 'docs_root'
        if parts[1] == 'api':
            return 'docs_api'
        if parts[1] == 'design':
            return 'docs_design'
    if parts[0] == 'python':
        if len(parts) == 2:
            return 'python_root'
        else:
            return 'python_engine3g'
    if parts[0] == 'scripts':
        if len(parts) == 2:
            return 'scripts_root'
        if parts[1] == 'build':
            return 'scripts_build'
        if parts[1] == 'ci':
            return 'scripts_ci'
    if str(path) == 'README.md':
        return 'module_engine'
    return 'placeholder_fill'


def generate_content(path: Path) -> str:
    if str(path) in MANUAL_CONTENT:
        data = MANUAL_CONTENT[str(path)]
        title = data['title']
        current = data['current']
        usage = data['usage']
        todo = TODO_TEXT[data['todo']]
    else:
        title = friendly_title(path)
        current = summarize_files(path)
        usage = usage_guidance(path, path.parent)
        todo = TODO_TEXT[todo_key_for(path)]

    lines = [f'# {title}', '', '## Current State', '']
    if current:
        lines.extend(current)
    else:
        lines.append('- Details forthcoming.')
    lines.extend(['', '## Usage', ''])
    if usage:
        lines.extend(usage)
    else:
        lines.append('- Usage guidance will be documented alongside the first implementation.')
    lines.extend(['', '## TODO / Next Steps', '', f'- {todo}', ''])
    return '\n'.join(lines).strip() + '\n'


def main() -> None:
    readmes = sorted(
        p for p in ROOT.rglob('README.md') if 'third_party' not in p.parts
    )
    for path in readmes:
        if str(path) == 'README.md':
            continue
        content = generate_content(path)
        path.write_text(content, encoding='utf-8')

    # Emit todo aggregation for review.
    todo_map: dict[str, list[str]] = {}
    for path in readmes:
        key = todo_key_for(path)
        todo_map.setdefault(key, []).append(str(path))
    for key, paths in sorted(todo_map.items()):
        print(key)
        for item in paths:
            print('  ', item)

if __name__ == '__main__':
    main()
