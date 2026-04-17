#version 420 core

layout(location = 0) in vec3 position;

uniform mat4 lightViewProj;
uniform mat4 modelMatrix;

uniform sampler2D terrainHeightTex;
uniform vec3 terrainCacheOriginWorld;
uniform float terrainVertexSpacing;
uniform int terrainCacheResolution;

// ----------------------------------------------------------------------------
// Compute the corresponding cache coordinate for a given world XZ position
// ----------------------------------------------------------------------------
ivec2 terrainCacheCoord(vec2 worldXZ)
{
    vec2 cacheSample = round((worldXZ - terrainCacheOriginWorld.xz) / terrainVertexSpacing);
    return clamp(ivec2(cacheSample), ivec2(0), ivec2(terrainCacheResolution - 1));
}

void main()
{
    vec3 localPos = position;
    vec4 baseWorldPos = modelMatrix * vec4(localPos, 1.0);
    ivec2 cacheCoord = terrainCacheCoord(baseWorldPos.xz);

    float height = texelFetch(terrainHeightTex, cacheCoord, 0).r;
    localPos.y += height;

    vec4 worldPos = modelMatrix * vec4(localPos, 1.0);
    gl_Position = lightViewProj * worldPos;
}
