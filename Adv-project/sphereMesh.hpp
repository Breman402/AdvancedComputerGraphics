#pragma once

#include <GL/glew.h>

struct SphereMesh
{
    GLuint vao        = 0;
    GLuint vbo        = 0;
    GLuint ebo        = 0;
    GLsizei indexCount = 0;
    float radius = 0.0f;
    int stacks = 0;
    int slices = 0;
};

SphereMesh createSphereMesh(float radius, int stacks, int slices);

// This is for updating with the GUI
void updateSphereMesh(SphereMesh& mesh);