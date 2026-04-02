#pragma once

#include <GL/glew.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

struct ShadowMap
{
    GLuint fbo       = 0;
    GLuint depthTex  = 0;
    int    width     = 0;
    int    height    = 0;
};

ShadowMap createShadowMap(int size);

void updateShadowMap(ShadowMap& map, int newSize);

void updateShadowMatrices(int cameraGridX, int cameraGridZ, const float terrainSquareSize, const float renderDistance, const float terrainMaxHeight, const glm::vec3& lightDirWorld, glm::mat4& lightViewMatrix, glm::mat4& lightProjectionMatrix);