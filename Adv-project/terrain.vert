#version 420 core

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 texcoord;

uniform mat4 modelViewProjectionMatrix;
uniform mat4 modelMatrix;
uniform mat4 lightViewProj;

uniform sampler2D terrainHeightTex;
uniform sampler2D terrainNormalTex;
uniform vec3 terrainCacheOriginWorld;
uniform float terrainVertexSpacing;
uniform int terrainCacheResolution;

out vec2 vTexcoord;
out vec3 vWorldPos;
out vec3 vWorldNormal;
out vec4 vShadowCoord;
out float vHeight;

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
    vTexcoord = texcoord;

    vec3 localPos = position;
    vec4 baseWorldPos = modelMatrix * vec4(localPos, 1.0);
    ivec2 cacheCoord = terrainCacheCoord(baseWorldPos.xz);

    float height = texelFetch(terrainHeightTex, cacheCoord, 0).r;
    vec2 normalXZ = texelFetch(terrainNormalTex, cacheCoord, 0).rg;
    float normalY = sqrt(max(0.0, 1.0 - dot(normalXZ, normalXZ)));

    localPos.y += height;

    vec4 worldPos = modelMatrix * vec4(localPos, 1.0);
    vWorldPos = worldPos.xyz;
    vWorldNormal = normalize(vec3(normalXZ.x, normalY, normalXZ.y));
    vHeight = height;

    gl_Position = modelViewProjectionMatrix * vec4(localPos, 1.0);
    vShadowCoord = lightViewProj * worldPos;
}
