#include "createShadow.hpp"
#include <cstdio>
#include <algorithm>

ShadowMap createShadowMap(int size)
{
    ShadowMap map{};
    map.width  = size;
    map.height = size;

    glGenFramebuffers(1, &map.fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, map.fbo);

    // Depth texture
    glGenTextures(1, &map.depthTex);
    glBindTexture(GL_TEXTURE_2D, map.depthTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24,
                 size, size, 0,
                 GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

    const float borderColor[] = {1.0f, 1.0f, 1.0f, 1.0f};
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

    // Attach depth texture
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                           GL_TEXTURE_2D, map.depthTex, 0);

    // No color output
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    {
        std::fprintf(stderr, "Shadow framebuffer not complete!\n");
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    return map;
}

void updateShadowMap(ShadowMap& map, int newSize)
{
    // Delete old resources
    if (map.depthTex)
        glDeleteTextures(1, &map.depthTex);
    if (map.fbo)
        glDeleteFramebuffers(1, &map.fbo);

    // Create new shadow map
    printf("Updating shadow map to size %d x %d\n", newSize, newSize);
    map = createShadowMap(newSize);
}

/**
 * @brief Updates the light view and projection matrices for shadow mapping.
 *
 * Calculates optimized view and projection matrices for directional shadow mapping
 * by fitting an orthographic projection around the camera's visible terrain area.
 * The light position is computed based on the light direction, and the projection
 * bounds are determined by transforming terrain corner points to light space.
 *
 * @param cameraGridX The camera's grid position X coordinate
 * @param cameraGridZ The camera's grid position Z coordinate
 * @param terrainSquareSize The size of each terrain square in world units
 * @param renderDistance The rendering distance in terrain squares
 * @param terrainMaxHeight The maximum height of the terrain in world units
 * @param lightDirWorld The directional light direction in world space
 * @param[out] lightViewMatrix The resulting view matrix for the light source
 * @param[out] lightProjectionMatrix The resulting orthographic projection matrix for shadow rendering
 *
 * @note The shadow projection is fitted tightly around visible terrain to minimize
 *       shadow map resolution waste and reduce aliasing artifacts.
 * @note An up vector of (0, 0, 1) is used when the light direction is nearly parallel
 *       to the world up vector (0, 1, 0) to avoid gimbal lock.
 */
void updateShadowMatrices(int cameraGridX, int cameraGridZ, const float terrainSquareSize, const float renderDistance, const float terrainMaxHeight, const glm::vec3& lightDirWorld, glm::mat4& lightViewMatrix, glm::mat4& lightProjectionMatrix)
{
    const float terrainHalfSpan = (renderDistance + 0.5f) * terrainSquareSize;
    const float padding = terrainSquareSize;

    const glm::vec3 shadowTarget(
        (cameraGridX + 0.5f) * terrainSquareSize,
        terrainMaxHeight * 0.5f,
        (cameraGridZ + 0.5f) * terrainSquareSize
    );

    const glm::vec3 lightDirection = glm::normalize(lightDirWorld);
    const glm::vec3 worldUp(0.0f, 1.0f, 0.0f);
    const glm::vec3 lightUp =
        std::abs(dot(lightDirection, worldUp)) > 0.98f ? glm::vec3(0.0f, 0.0f, 1.0f) : worldUp;

    const float lightDistance = terrainHalfSpan * 2.0f + terrainMaxHeight + padding;
    const glm::vec3 lightPos = shadowTarget - lightDirection * lightDistance;

    lightViewMatrix = lookAt(lightPos, shadowTarget, lightUp);

    float minX = std::numeric_limits<float>::max();
    float maxX = std::numeric_limits<float>::lowest();
    float minY = std::numeric_limits<float>::max();
    float maxY = std::numeric_limits<float>::lowest();
    float minZ = std::numeric_limits<float>::max();
    float maxZ = std::numeric_limits<float>::lowest();

    for (int xSign : {-1, 1})
    {
        for (int zSign : {-1, 1})
        {
            for (int ySign : {0, 1})
            {
                const glm::vec3 cornerWorld(
                    shadowTarget.x + xSign * terrainHalfSpan,
                    ySign ? terrainMaxHeight : 0.0f,
                    shadowTarget.z + zSign * terrainHalfSpan
                );

                const glm::vec3 cornerLight = glm::vec3(lightViewMatrix * glm::vec4(cornerWorld, 1.0f));
                minX = std::min(minX, cornerLight.x);
                maxX = std::max(maxX, cornerLight.x);
                minY = std::min(minY, cornerLight.y);
                maxY = std::max(maxY, cornerLight.y);
                minZ = std::min(minZ, cornerLight.z);
                maxZ = std::max(maxZ, cornerLight.z);
            }
        }
    }

    const float nearPlane = std::max(0.1f, -maxZ - padding);
    const float farPlane = std::max(nearPlane + 1.0f, -minZ + padding);

    lightProjectionMatrix = glm::ortho(
        minX - padding, maxX + padding,
        minY - padding, maxY + padding,
        nearPlane, farPlane
    );
}