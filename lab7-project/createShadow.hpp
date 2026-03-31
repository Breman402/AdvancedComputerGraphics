#pragma once

#include <GL/glew.h>


struct ShadowMap
{
    GLuint fbo       = 0;
    GLuint depthTex  = 0;
    int    width     = 0;
    int    height    = 0;
};

ShadowMap createShadowMap(int size);

void updateShadowMap(ShadowMap& map, int newSize);