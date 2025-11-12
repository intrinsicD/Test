---
id: 999
title: Fix Geometry Viewer Black Screen - Enable Cube Rendering and Camera Controls
status: in_progress
priority: P0
area: rendering
size: L
owner: rendering-lead
gates: [manual_test]
relates_to: [bundle:rendering]
blocked_on: []
links: []
---

## Intent

Fix the geometry viewer application to properly render a procedural cube with working orbit camera controls instead of showing a black screen.

## Context

The geometry viewer application compiles and runs but only shows a black window. The cube mesh is generated procedurally but isn't visible. User wants to:
1. See the cube rendered
2. Rotate the camera using mouse drag
3. Zoom using mouse scroll

### Current State
- Application starts without crashes
- Window is created successfully  
- Mesh is generated (procedural cube)
- Camera is set up
- But screen remains black - nothing renders

### Root Causes Identified

1. **GLAD Not Initialized**: Log shows `ENGINE_RENDERING_HAS_GLAD=0` - OpenGL function pointers not loaded
2. **No Shader Program**: Material system has no actual shader program attached
3. **Research Baseline Incomplete**: The research baseline pipeline doesn't have working shaders
4. **Missing OpenGL Context Initialization**: GLAD needs to be initialized after OpenGL context creation
5. **No Clear Color**: Framebuffer might not be cleared properly

## Design/Plan

### Phase 1: Enable GLAD and OpenGL Context
1. Ensure GLAD is built and linked
2. Initialize GLAD after GLFW creates OpenGL context
3. Verify OpenGL version and capabilities

### Phase 2: Implement Basic Shader
1. Create simple vertex + fragment shader for forward rendering
2. Handle MVP matrix uniforms
3. Add basic lighting (or flat shading to start)

### Phase 3: Wire Up Rendering Pipeline  
1. Ensure mesh data is uploaded to GPU
2. Connect shader to material system
3. Verify draw calls are submitted

### Phase 4: Fix Camera
1. Verify camera matrices are correct
2. Ensure camera updates propagate to shaders
3. Test mouse input for orbit controls

## Steps

### Step 1: Fix GLAD Build and Initialization

**File**: `engine/rendering/src/backend/opengl/presentation_backend.cpp`

Add GLAD initialization in `initialize_context_if_needed`:

```cpp
void OpenGLPresentationBackend::initialize_context_if_needed(void* window_handle)
{
    if (context_initialized_ && current_window_ == window_handle)
    {
        return;
    }

    auto* glfw_window = static_cast<GLFWwindow*>(window_handle);
    glfwMakeContextCurrent(glfw_window);

#if ENGINE_RENDERING_HAS_GLAD
    if (!gladLoadGL(glfwGetProcAddress))
    {
        throw std::runtime_error("Failed to initialize GLAD");
    }
    ENGINE_INFO("OpenGL {}.{} initialized", GLVersion.major, GLVersion.minor);
#else
    ENGINE_WARN("GLAD not available - OpenGL functions may not work");
#endif

    context_initialized_ = true;
    current_window_ = window_handle;
}
```

### Step 2: Create Basic Forward Shader

**File**: `engine/rendering/src/pipeline/research_baseline.cpp`

Add shader source code:

```cpp
namespace
{
    const char* FORWARD_VERTEX_SHADER = R"(
        #version 460 core
        
        layout(location = 0) in vec3 aPosition;
        layout(location = 1) in vec3 aNormal;
        
        uniform mat4 uModel;
        uniform mat4 uView;
        uniform mat4 uProjection;
        
        out vec3 vNormal;
        out vec3 vFragPos;
        
        void main()
        {
            vec4 worldPos = uModel * vec4(aPosition, 1.0);
            vFragPos = worldPos.xyz;
            vNormal = mat3(transpose(inverse(uModel))) * aNormal;
            gl_Position = uProjection * uView * worldPos;
        }
    )";
    
    const char* FORWARD_FRAGMENT_SHADER = R"(
        #version 460 core
        
        in vec3 vNormal;
        in vec3 vFragPos;
        
        out vec4 FragColor;
        
        uniform vec3 uLightPos;
        uniform vec3 uViewPos;
        uniform vec3 uObjectColor;
        
        void main()
        {
            // Simple Blinn-Phong lighting
            vec3 normal = normalize(vNormal);
            vec3 lightDir = normalize(uLightPos - vFragPos);
            vec3 viewDir = normalize(uViewPos - vFragPos);
            vec3 halfDir = normalize(lightDir + viewDir);
            
            // Ambient
            float ambientStrength = 0.3;
            vec3 ambient = ambientStrength * uObjectColor;
            
            // Diffuse
            float diff = max(dot(normal, lightDir), 0.0);
            vec3 diffuse = diff * uObjectColor;
            
            // Specular
            float spec = pow(max(dot(normal, halfDir), 0.0), 32.0);
            vec3 specular = vec3(0.5) * spec;
            
            vec3 result = ambient + diffuse + specular;
            FragColor = vec4(result, 1.0);
        }
    )";
}
```

### Step 3: Compile and Link Shader Program

Add shader compilation helper and store compiled program in research baseline resources.

### Step 4: Upload Mesh to GPU

Ensure vertex buffers are created and mesh data is uploaded in the OpenGL resource provider.

### Step 5: Submit Draw Calls

Verify the forward geometry pass actually issues `glDrawElements` or `glDrawArrays`.

### Step 6: Set Uniforms

Before each draw call, set MVP matrices and material properties.

### Step 7: Enable Depth Testing

```cpp
glEnable(GL_DEPTH_TEST);
glDepthFunc(GL_LESS);
```

### Step 8: Clear Framebuffer with Visible Color

```cpp
glClearColor(0.1f, 0.1f, 0.15f, 1.0f); // Dark blue-gray
glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
```

## Testing

### Manual Test Cases

1. **Application Starts**
   - Run `./geometry_viewer`
   - Window should open with dark blue-gray background
   - No crashes or errors in log

2. **Cube Renders**
   - Cube should be visible in center of window
   - Cube should have proper lighting/shading
   - All faces should be visible

3. **Camera Orbit**
   - Click and drag with left mouse button
   - Camera should rotate around cube
   - Rotation should be smooth

4. **Camera Zoom**
   - Scroll mouse wheel
   - Camera should move closer/farther from cube
   - Zoom should be clamped to reasonable limits

5. **Escape to Quit**
   - Press ESC key
   - Application should exit cleanly

## Evidence

_To be filled after implementation and testing_

## Notes

- GLAD must be initialized AFTER OpenGL context is created by GLFW
- Shader compilation errors should be logged with full info log
- Consider adding wireframe mode toggle for debugging
- May need to check if geometry has correct vertex format (positions + normals)

