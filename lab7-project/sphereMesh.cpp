#include "sphereMesh.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <vector>

SphereMesh createSphereMesh(float radius, int stacks, int slices)
{
    struct Vertex
    {
        glm::vec3 position;
        glm::vec3 normal;
        glm::vec2 texcoord;
    };

    std::vector<Vertex>       vertices;
    std::vector<unsigned int> indices;

    const float PI     = glm::pi<float>();
    const float TWO_PI = 2.0f * PI;

    // -------------------------------
    // Generate vertices (UVs with east–west flip)
    // -------------------------------
    for (int i = 0; i <= stacks; ++i)
    {
        float v   = float(i) / float(stacks);      // 0..1
        float lat = PI * (v - 0.5f);               // -pi/2..+pi/2

        float y = radius * std::sin(lat);
        float r = radius * std::cos(lat);

        for (int j = 0; j <= slices; ++j)
        {
            float u   = float(j) / float(slices);  // 0..1
            float lon = TWO_PI * u;                // 0..2pi

            float x = r * std::cos(lon);
            float z = r * std::sin(lon);

            glm::vec3 pos = glm::vec3(x, y, z);
            glm::vec3 nor = glm::normalize(pos);

            // Flip U horizontally: u -> 1.0 - u
            glm::vec2 tex = glm::vec2(1.0f - u, 1.0f - v);

            vertices.push_back({ pos, nor, tex });
        }
    }

    // -------------------------------
    // Indices
    // -------------------------------
    for (int i = 0; i < stacks; ++i)
    {
        for (int j = 0; j < slices; ++j)
        {
            int row1 = i * (slices + 1);
            int row2 = (i + 1) * (slices + 1);

            indices.push_back(row1 + j);
            indices.push_back(row2 + j);
            indices.push_back(row1 + j + 1);

            indices.push_back(row1 + j + 1);
            indices.push_back(row2 + j);
            indices.push_back(row2 + j + 1);
        }
    }

    // -------------------------------
    // Upload to OpenGL
    // -------------------------------
    SphereMesh mesh{};
    mesh.indexCount = static_cast<GLsizei>(indices.size());
    mesh.radius = radius;
    mesh.stacks = stacks;
    mesh.slices = slices;

    glGenVertexArrays(1, &mesh.vao);
    glBindVertexArray(mesh.vao);

    glGenBuffers(1, &mesh.vbo);
    glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo);
    glBufferData(GL_ARRAY_BUFFER,
                 vertices.size() * sizeof(Vertex),
                 vertices.data(),
                 GL_STATIC_DRAW);

    glGenBuffers(1, &mesh.ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 indices.size() * sizeof(unsigned int),
                 indices.data(),
                 GL_STATIC_DRAW);

    // position
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE,
                          sizeof(Vertex), (void*)offsetof(Vertex, position));

    // normal
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE,
                          sizeof(Vertex), (void*)offsetof(Vertex, normal));

    // texcoord
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE,
                          sizeof(Vertex), (void*)offsetof(Vertex, texcoord));

    glBindVertexArray(0);

    return mesh;
}

void updateSphereMesh(SphereMesh& mesh)
{
    // First delete old buffers
    if (mesh.ebo) glDeleteBuffers(1, &mesh.ebo);
    if (mesh.vbo) glDeleteBuffers(1, &mesh.vbo);
    if (mesh.vao) glDeleteVertexArrays(1, &mesh.vao);

    // Create new sphere mesh
    SphereMesh newMesh = createSphereMesh(mesh.radius, mesh.stacks, mesh.slices);

    // Update the existing mesh struct
    mesh = newMesh;
}