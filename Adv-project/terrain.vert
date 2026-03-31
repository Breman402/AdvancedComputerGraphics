#version 410 core

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 texcoord;

// Matrices
uniform mat4 modelViewProjectionMatrix;
uniform mat4 modelMatrix;
uniform mat4 lightViewProj;

// Terrain controls
uniform float heightScale;   // user-controlled multiplier, typically around [0, 1]
uniform float tileWidth;     // world-space width of one tile
uniform float tileHeight;    // hard maximum terrain height
uniform int tileSeed;

// Outputs to fragment shader
out vec2 vTexcoord;
out vec3 vWorldPos;
out vec3 vWorldNormal;
out vec4 vShadowCoord;
out float vHeight; // pass height to fragment shader for texture blending

// -----------------------------------------------------------------------------
// Pseudo-random hash
// -----------------------------------------------------------------------------
float hash(vec2 p)
{
    p += vec2(float(tileSeed) * 0.1234, float(tileSeed) * 0.5678);
    return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453123);
}

// -----------------------------------------------------------------------------
// Smooth value noise
// -----------------------------------------------------------------------------
float valueNoise(vec2 p)
{
    vec2 i = floor(p);
    vec2 f = fract(p);

    float a = hash(i);
    float b = hash(i + vec2(1.0, 0.0));
    float c = hash(i + vec2(0.0, 1.0));
    float d = hash(i + vec2(1.0, 1.0));

    vec2 u = f * f * (3.0 - 2.0 * f);

    return mix(
        mix(a, b, u.x),
        mix(c, d, u.x),
        u.y
    );
}

// -----------------------------------------------------------------------------
// Fractal Brownian Motion
// -----------------------------------------------------------------------------
float fbm(vec2 p)
{
    float sum = 0.0;
    float amplitude = 0.5;
    float frequency = 1.0;

    for (int i = 0; i < 5; ++i)
    {
        sum += amplitude * valueNoise(p * frequency);
        frequency *= 2.0;
        amplitude *= 0.5;
    }

    return sum;
}

// -----------------------------------------------------------------------------
// Terrain height
// heightScale = user multiplier
// tileHeight  = hard max
// final result is clamped to [0, tileHeight]
// -----------------------------------------------------------------------------
float getTerrainHeight(vec2 worldXZ)
{
    float noiseScale = 0.03;

    float h = fbm(worldXZ * noiseScale);   // roughly [0,1]
    h = smoothstep(0.2, 0.8, h);           // shape terrain a bit
    h = clamp(h * heightScale, 0.0, 1.0);  // keep within normalized range

    return h * tileHeight;                 // final hard maximum
}

void main()
{
    vTexcoord = texcoord;

    // Base local vertex
    vec3 localPos = position;

    // Convert base vertex to world space before sampling terrain,
    // so neighboring tiles line up continuously.
    vec4 baseWorldPos = modelMatrix * vec4(localPos, 1.0);

    // Sample procedural terrain in world space
    float height = getTerrainHeight(baseWorldPos.xz);

    // Displace upward
    vec3 displacedLocalPos = localPos;
    displacedLocalPos.y += height;

    // Final world position
    vec4 worldPos = modelMatrix * vec4(displacedLocalPos, 1.0);
    vWorldPos = worldPos.xyz;
    vHeight = height;

    // Approximate terrain normal from nearby height samples
    float eps = tileWidth / 128.0;

    float hL = getTerrainHeight(baseWorldPos.xz + vec2(-eps, 0.0));
    float hR = getTerrainHeight(baseWorldPos.xz + vec2( eps, 0.0));
    float hD = getTerrainHeight(baseWorldPos.xz + vec2(0.0, -eps));
    float hU = getTerrainHeight(baseWorldPos.xz + vec2(0.0,  eps));

    vec3 dx = vec3(2.0 * eps, hR - hL, 0.0);
    vec3 dz = vec3(0.0, hU - hD, 2.0 * eps);

    vWorldNormal = normalize(cross(dz, dx));

    gl_Position = modelViewProjectionMatrix * vec4(displacedLocalPos, 1.0);
    vShadowCoord = lightViewProj * worldPos;
}