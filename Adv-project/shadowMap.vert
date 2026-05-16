#version 420 core

layout(location = 0) in vec3 position;
layout(location = 2) in vec2 texCoord;

uniform mat4 lightViewProj;
uniform mat4 modelMatrix;

uniform sampler2D terrainHeightTex;
uniform vec3 terrainCacheOriginWorld;
uniform float terrainVertexSpacing;
uniform int terrainCacheResolution;
uniform float tileHeight;
uniform float minTerrainHeight01;
uniform float maxTerrainHeight01;
uniform float heightOffset;
uniform bool scatterShadowMode = false;

out vec2 vTexCoord;
flat out int vTerrainAllowed;

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
    vTexCoord = texCoord;
    vTerrainAllowed = 1;

    if (scatterShadowMode)
    {
        vec4 worldPos = modelMatrix * vec4(position, 1.0);
        vec4 instanceOriginWorld = modelMatrix * vec4(0.0, 0.0, 0.0, 1.0);
        ivec2 cacheCoord = terrainCacheCoord(instanceOriginWorld.xz);
        float height = texelFetch(terrainHeightTex, cacheCoord, 0).r;
        float height01 = clamp(height / tileHeight, 0.0, 1.0);

        vTerrainAllowed = int(height01 >= minTerrainHeight01 &&
                              height01 <= maxTerrainHeight01);

        worldPos.y += height + heightOffset;
        gl_Position = lightViewProj * worldPos;
        return;
    }

    vec3 localPos = position;
    vec4 baseWorldPos = modelMatrix * vec4(localPos, 1.0);
    ivec2 cacheCoord = terrainCacheCoord(baseWorldPos.xz);
    float height = texelFetch(terrainHeightTex, cacheCoord, 0).r;
    localPos.y += height;

    vec4 worldPos = modelMatrix * vec4(localPos, 1.0);
    gl_Position = lightViewProj * worldPos;
}
