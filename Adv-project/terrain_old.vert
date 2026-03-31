#version 410 core

// ----------------------------------------------------------------------------
// Vertex Attributes
// ----------------------------------------------------------------------------
layout(location = 0) in vec3 position;   // Base mesh vertex position (unit sphere)
layout(location = 1) in vec3 normal;     // Base vertex normal (unused for height)
layout(location = 2) in vec2 texcoord;   // UV coordinates for color/height lookup

// ----------------------------------------------------------------------------
// Uniforms: Matrices
// ----------------------------------------------------------------------------
uniform mat4 modelViewProjectionMatrix;
uniform mat4 modelMatrix;



// ----------------------------------------------------------------------------
// Uniforms: Terrain
// ----------------------------------------------------------------------------
uniform float heightScale;         // How much mountains are displaced outward
uniform float tileWidth;
uniform float tileHeight;
uniform int tileSeed;

// ----------------------------------------------------------------------------
// Varyings passed to fragment shader
// ----------------------------------------------------------------------------
out vec2 vTexcoord;
out vec3 vWorldPos;
out vec3 vWorldNormal;

// ----------------------------------------------------------------------------
// shadow coord
// ----------------------------------------------------------------------------
out vec4 vShadowCoord;
uniform mat4 lightViewProj;

float getTerrainHeight(vec2 uv)
{
    // Simple procedural height based on UV and tile seed
    float height = 0.0;
    height += 0.5 * sin(uv.x * gridResolution + tileSeed);
    height += 0.5 * cos(uv.y * gridResolution + tileSeed);
    return height * heightScale;
}

void main()
{
    // Pass UVs directly to the shader
    vTexcoord = texcoord;

    // ============================================================================
    // Terrain Generation
    // ============================================================================
    // Calculate height displacement using a procedural function based on seed
    float height = getTerrainHeight(seed);
    // Displace vertex along its normal (which is up for a flat plane)
    vec3 displacedLocalPos = position + normal * height;

    // ============================================================================
    // LIGHTING INPUT PREPARATION (world-space position & normal)
    // ============================================================================

    // Convert displaced position to WORLD space
    vec4 worldPos = modelMatrix * vec4(displacedLocalPos, 1.0);
    vWorldPos = worldPos.xyz;

    // The mesh normal already points upward for a flat terrain tile.
    vWorldNormal = normalize(mat3(modelMatrix) * normal);

    // ----------------------------------------------------------------------------
    // Final clip-space output
    // ----------------------------------------------------------------------------
    gl_Position = modelViewProjectionMatrix * vec4(displacedLocalPos, 1.0);

    // Calculate shadow coordinates
    vShadowCoord = lightViewProj * modelMatrix * vec4(displacedLocalPos, 1.0);
}

