#include "loadTexture.hpp"

#include "stb_image.h"

#include <iostream>

GLuint loadTexture(const std::string& path)
{
    int width, height, channels;
    bool useAnisotropicFiltering = true;
    bool useGL_NEAREST = true;
    GLfloat largestSupportedAnisotropy = 0.0f;

    stbi_set_flip_vertically_on_load(false);
    unsigned char* data = stbi_load(path.c_str(), &width, &height, &channels, 3);

    if (!data)
    {
        std::cerr << "Failed to load texture: " << path << std::endl;
        std::cerr << "stb_image error: " << stbi_failure_reason() << std::endl;
        return 0;
    }

    GLuint tex = 0;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1); // Set pixel alignment to 1 byte (for tightly packed data)
    
    glTexImage2D(
        GL_TEXTURE_2D, 0, GL_RGB8,
        width, height, 0,
        GL_RGB, GL_UNSIGNED_BYTE, data
    );

    glGenerateMipmap(GL_TEXTURE_2D);

    // Filtering
    if (useGL_NEAREST)
    {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    }
    else
    {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    }
    if (useAnisotropicFiltering)
    {
        glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &largestSupportedAnisotropy);
        // Set the maximum anisotropy level supported by the GPU
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, largestSupportedAnisotropy);
    }

    // Wrapping
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    stbi_image_free(data);

    return tex;
}

