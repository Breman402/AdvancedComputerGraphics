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
// So the way that I've decided to have this done is to first do one pass
// to get some base height and then use that to have diffrent levels of 
// detail for the diffrent terrain heights, like you'd expect to see more detail
// in the rocks and snow than the sand.
// -----------------------------------------------------------------------------
float fbm(vec2 p)
{
    float sum = 0.0;
    float amplitude = 0.5;
    float frequency = 1.0;

    for (int i = 0; i < 20; ++i)
    {
        sum += amplitude * valueNoise(p * frequency);
        frequency *= 2.0;
        amplitude *= 0.5;
    }

    return sum;
}

float baseHeight(vec2 worldXZ)
{
    float h = fbm(worldXZ * 0.003);
    h = smoothstep(0.35, 0.85, h);
    return h;
}

float fbmCustom(vec2 p, float startFreq, float freqMul, float startAmp, float ampMul, int maxOctaves = 100)
{
    float sum = 0.0;
    float amplitude = startAmp;
    float frequency = startFreq;

    for (int i = 0; i < maxOctaves; ++i)
    {
        if (amplitude < 0.0005) break;
        sum += amplitude * valueNoise(p * frequency);
        frequency *= freqMul;
        amplitude *= ampMul;
    }

    return sum;
}

// -----------------------------------------------------------------------------
// Terrain height
// heightScale = user multiplier
// tileHeight  = hard max
// -----------------------------------------------------------------------------
float getTerrainHeight(vec2 worldXZ)
{
    float base = baseHeight(worldXZ);

    float shoreMask = 1.0 - smoothstep(0.10, 0.20, base);
    float grassMask = smoothstep(0.15, 0.30, base) * (1.0 - smoothstep(0.45, 0.60, base));
    float rockMask  = smoothstep(0.50, 0.65, base) * (1.0 - smoothstep(0.75, 0.88, base));
    float snowMask  = smoothstep(0.80, 0.95, base);

    float detail = 0.0;

    if (shoreMask > 0.001)
        detail += fbmCustom(worldXZ * 0.008, 1.0, 1.8, 0.35, 0.5, 8) * shoreMask;

    if (grassMask > 0.001)
        detail += fbmCustom(worldXZ * 0.012, 1.0, 2.0, 0.45, 0.5, 8) * grassMask;

    if (rockMask > 0.001)
        detail += fbmCustom(worldXZ * 0.020, 1.0, 2.3, 0.5, 0.5, 10) * rockMask;

    if (snowMask > 0.001)
        detail += fbmCustom(worldXZ * 0.015, 1.0, 1.7, 0.25, 0.5, 8) * snowMask;

    float h = base + 0.25 * detail;
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
