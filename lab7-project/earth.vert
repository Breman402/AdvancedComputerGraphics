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
// Uniforms: Heightmap Displacement, these are set in the main file using the
// set unifrom slow from lab helper
// ----------------------------------------------------------------------------
uniform sampler2D earthHeightTex;  // Heightmap texture
uniform float baseRadius;          // Base radius of the Earth
uniform float heightScale;         // How much mountains are displaced outward

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

void main()
{
    // Pass UVs directly to the shader
    vTexcoord = texcoord;

    // ============================================================================
    // HEIGHTMAP DISPLACEMENT LOGIC
    // ============================================================================

    // Unit direction from sphere center
    vec3 dir = normalize(position);

    // Sample height value [0,1] from heightmap texture
    float h = texture(earthHeightTex, texcoord).r; // This samples the texture and takes the red channel
                                                   // from the RGBA texture which is what is used for the heightmap
                                                   // still works if it is .g, but .r is used to indicate that it
                                                   // is a grayscale heightmap

    // Compute displaced radius = base + scaled relief
    float r = baseRadius + h * heightScale; // This is only used if some other radius than 1.0 is used,
                                            // right now I don't plan on making the radius variable

    // Position in MODEL space after displacement
    vec3 displacedLocalPos = dir * r;

    // ============================================================================
    // LIGHTING INPUT PREPARATION (world-space position & normal)
    // ============================================================================

    // Convert displaced position to WORLD space
    vec4 worldPos = modelMatrix * vec4(displacedLocalPos, 1.0);
    vWorldPos = worldPos.xyz;

    // Normal is along 'dir', transformed by model rotation
    vWorldNormal = normalize(mat3(modelMatrix) * dir);

    // ----------------------------------------------------------------------------
    // Final clip-space output
    // ----------------------------------------------------------------------------
    gl_Position = modelViewProjectionMatrix * vec4(displacedLocalPos, 1.0);

    // Calculate shadow coordinates
    vShadowCoord = lightViewProj * modelMatrix * vec4(displacedLocalPos, 1.0);
}
