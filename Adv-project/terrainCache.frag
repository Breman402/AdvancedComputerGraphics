#version 420 core

layout(location = 0) out float outHeight;
layout(location = 1) out vec2 outNormalXZ;

uniform vec3 terrainCacheOriginWorld;
uniform float terrainVertexSpacing;
uniform float heightScale;
uniform float tileHeight;
uniform int tileSeed;

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

    for (int i = 0; i < 100; ++i)
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
    const float noiseScale = 0.008;

    float h = fbm(worldXZ * noiseScale);
    h = smoothstep(0.55, 0.9, h);
    h = clamp(h * heightScale, 0.0, 1.0);

    return h * tileHeight;
}

void main()
{
    ivec2 texelCoord = ivec2(gl_FragCoord.xy);
    vec2 worldXZ = terrainCacheOriginWorld.xz + vec2(texelCoord) * terrainVertexSpacing;

    float height = getTerrainHeight(worldXZ);

    vec3 worldPos = vec3(worldXZ.x, height, worldXZ.y);

    vec3 dx = dFdx(worldPos);
    vec3 dz = dFdy(worldPos);
    vec3 worldNormal = normalize(cross(dz, dx));

    outHeight = height;
    outNormalXZ = worldNormal.xz;
}
