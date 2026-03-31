#pragma once

#include <GL/glew.h>
#include <labhelper.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <imgui.h>
#include <imgui_impl_sdl_gl3.h>

#include <map>
#include <vector>

using namespace glm;

#define SQUARE_SIZE 32

struct TerrainSquare
{
    vec2 center; // world position of the center of the square, acts as an ID and coordinates
    GLuint texture = 0;
    GLuint heightTexture = 0;
    int width = SQUARE_SIZE; // width of the square in world units
    int height = SQUARE_SIZE; // height of the square in world units
};


void generateHeightmap(const char* heightmapPath, TerrainSquare& square);