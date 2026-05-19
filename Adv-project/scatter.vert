#version 420 core

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 texCoord;

uniform mat4 modelMatrix;
uniform mat4 viewProjectionMatrix;
uniform mat4 lightViewProj;
uniform sampler2D terrainHeightTex;
uniform vec3 terrainCacheOriginWorld;
uniform float terrainVertexSpacing;
uniform int terrainCacheResolution;
uniform float tileHeight;
uniform float minTerrainHeight01;
uniform float maxTerrainHeight01;
uniform float heightOffset;

out vec3 vWorldNormal;
out vec3 vWorldPos;
out vec4 vShadowCoord;
out vec2 vTexCoord;
flat out int vTerrainAllowed;

ivec2 terrainCacheCoord(vec2 worldXZ)
{
    vec2 cacheSample = round((worldXZ - terrainCacheOriginWorld.xz) / terrainVertexSpacing);
    return clamp(ivec2(cacheSample), ivec2(0), ivec2(terrainCacheResolution - 1));
}

void main()
{
    vec4 worldPos = modelMatrix * vec4(position, 1.0);
    vec4 instanceOriginWorld = modelMatrix * vec4(0.0, 0.0, 0.0, 1.0);
    ivec2 cacheCoord = terrainCacheCoord(instanceOriginWorld.xz);
    float terrainHeight = texelFetch(terrainHeightTex, cacheCoord, 0).r;
    float terrainHeight01 = clamp(terrainHeight / tileHeight, 0.0, 1.0);
    vTerrainAllowed = int(terrainHeight01 >= minTerrainHeight01 &&
                          terrainHeight01 <= maxTerrainHeight01);
    worldPos.y += terrainHeight + heightOffset;

    vWorldNormal = normalize(mat3(transpose(inverse(modelMatrix))) * normal);
    vWorldPos = worldPos.xyz;
    vShadowCoord = lightViewProj * worldPos;
    vTexCoord = texCoord;
    gl_Position = viewProjectionMatrix * worldPos;
}
