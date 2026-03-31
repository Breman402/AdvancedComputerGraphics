#include "createShadow.hpp"
#include <cstdio>

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