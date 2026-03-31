#pragma once
#include <GL/glew.h>
#include <string>

/**
 * @brief Load a 2D texture from disk using stb_image.
 *
 * @param path Path to an image file (jpg, png, etc.)
 * @return GLuint The OpenGL texture ID (0 if loading failed).
 */
GLuint loadTexture(const std::string& path);
