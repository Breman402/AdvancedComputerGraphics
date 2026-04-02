#version 420

layout(location = 0) in vec3 position;

uniform mat4 lightViewProj;
uniform mat4 modelMatrix;

uniform float heightScale;
uniform float tileWidth;
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
    vec3 localPos = position;

    // Flat vertex transformed to world space to sample continuous terrain
    vec4 baseWorldPos = modelMatrix * vec4(localPos, 1.0);

    // Same displacement as main terrain shader
    float height = getTerrainHeight(baseWorldPos.xz);
    localPos.y += height;

    // Final displaced world position
    vec4 worldPos = modelMatrix * vec4(localPos, 1.0);

    // Output light clip-space position for depth rendering
    gl_Position = lightViewProj * worldPos;
}
