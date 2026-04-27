#pragma once

#include <cstddef>
#include <glm/glm.hpp>
#include <labhelper.h>
#include <string>
#include <vector>
#include "createShadow.hpp"

class Material
{
public:
    glm::vec3 color;
    float metalness;
    float fresnel;
    float shininess;
    glm::vec3 emission;

    Material(const glm::vec3& color, float metalness, float fresnel, float shininess, const glm::vec3& emission)
        : color(color), metalness(metalness), fresnel(fresnel), shininess(shininess), emission(emission) {}

    void setMaterialUniform(const GLuint& shaderProgram, const std::string& uniformName) const
    {
        labhelper::setUniformSlow(shaderProgram, (uniformName + ".color").c_str(), color);
        labhelper::setUniformSlow(shaderProgram, (uniformName + ".metalness").c_str(), metalness);
        labhelper::setUniformSlow(shaderProgram, (uniformName + ".fresnel").c_str(), fresnel);
        labhelper::setUniformSlow(shaderProgram, (uniformName + ".shininess").c_str(), shininess);
        labhelper::setUniformSlow(shaderProgram, (uniformName + ".emission").c_str(), emission);
    }
};

/**
 * @brief Reusable GPU mesh for a single flat terrain tile.
 *
 * The mesh is generated as a regular grid in local space where the tile spans
 * `[-0.5, 0.5]` in both the x and z directions. The terrain and shadow shaders
 * can then reuse the same uploaded geometry while applying their own
 * displacement in the shader stage.
 */
class TerrainTileMesh
{
public:
    TerrainTileMesh() = default;
    TerrainTileMesh(const TerrainTileMesh&) = delete;
    TerrainTileMesh& operator=(const TerrainTileMesh&) = delete;

    /**
     * @brief Generates the tile geometry and uploads it to OpenGL buffers.
     * @param gridResolution Number of quads along one edge of the terrain tile.
     */
    void create(const int gridResolution)
    {
        if (gridResolution <= 0)
        {
            labhelper::fatal_error("TerrainTileMesh requires a positive grid resolution.");
        }

        if (m_vao != 0)
        {
            labhelper::fatal_error("TerrainTileMesh::create may only be called once per instance.");
        }

        m_gridResolution = gridResolution;

        std::vector<Vertex> vertices;
        std::vector<unsigned int> indices;

        vertices.reserve((gridResolution + 1) * (gridResolution + 1));
        indices.reserve(gridResolution * gridResolution * 6);

        for (int z = 0; z <= gridResolution; ++z)
        {
            for (int x = 0; x <= gridResolution; ++x)
            {
                const float u = float(x) / float(gridResolution);
                const float v = float(z) / float(gridResolution);

                Vertex vertex{};
                vertex.position = glm::vec3(u - 0.5f, 0.0f, v - 0.5f);
                vertex.normal = glm::vec3(0.0f, 1.0f, 0.0f);
                vertex.texcoord = glm::vec2(u, v);
                vertices.push_back(vertex);
            }
        }

        for (int z = 0; z < gridResolution; ++z)
        {
            for (int x = 0; x < gridResolution; ++x)
            {
                const unsigned int topLeft = z * (gridResolution + 1) + x;
                const unsigned int topRight = topLeft + 1;
                const unsigned int bottomLeft = (z + 1) * (gridResolution + 1) + x;
                const unsigned int bottomRight = bottomLeft + 1;

                indices.push_back(topLeft);
                indices.push_back(bottomLeft);
                indices.push_back(topRight);

                indices.push_back(topRight);
                indices.push_back(bottomLeft);
                indices.push_back(bottomRight);
            }
        }

        m_indexCount = static_cast<GLsizei>(indices.size());

        glGenVertexArrays(1, &m_vao);
        glBindVertexArray(m_vao);

        glGenBuffers(1, &m_vbo);
        glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
        glBufferData(
            GL_ARRAY_BUFFER,
            static_cast<GLsizeiptr>(vertices.size() * sizeof(Vertex)),
            vertices.data(),
            GL_STATIC_DRAW
        );

        glGenBuffers(1, &m_ebo);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ebo);
        glBufferData(
            GL_ELEMENT_ARRAY_BUFFER,
            static_cast<GLsizeiptr>(indices.size() * sizeof(unsigned int)),
            indices.data(),
            GL_STATIC_DRAW
        );

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(
            0,
            3,
            GL_FLOAT,
            GL_FALSE,
            sizeof(Vertex),
            reinterpret_cast<void*>(offsetof(Vertex, position))
        );

        glEnableVertexAttribArray(1);
        glVertexAttribPointer(
            1,
            3,
            GL_FLOAT,
            GL_FALSE,
            sizeof(Vertex),
            reinterpret_cast<void*>(offsetof(Vertex, normal))
        );

        glEnableVertexAttribArray(2);
        glVertexAttribPointer(
            2,
            2,
            GL_FLOAT,
            GL_FALSE,
            sizeof(Vertex),
            reinterpret_cast<void*>(offsetof(Vertex, texcoord))
        );

        glBindVertexArray(0);
    }

    /**
     * @brief Creates the GPU textures and framebuffer used for cached terrain data.
     * @param cacheResolution Number of cached terrain samples per axis.
     */
    void createCache(const int cacheResolution)
    {
        if (cacheResolution <= 0)
        {
            labhelper::fatal_error("TerrainTileMesh::createCache requires a positive cache resolution.");
        }

        if (m_gridResolution <= 0)
        {
            labhelper::fatal_error("TerrainTileMesh::create must be called before createCache.");
        }

        if (m_cacheFbo != 0)
        {
            labhelper::fatal_error("TerrainTileMesh::createCache may only be called once per instance.");
        }

        GLint maxTextureSize = 0;
        glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxTextureSize);
        if (cacheResolution > maxTextureSize)
        {
            labhelper::fatal_error("The terrain cache resolution exceeds GL_MAX_TEXTURE_SIZE.");
        }

        m_cacheResolution = cacheResolution;

        glGenFramebuffers(1, &m_cacheFbo);
        glBindFramebuffer(GL_FRAMEBUFFER, m_cacheFbo);

        glGenTextures(1, &m_heightTexture);
        configureCacheTexture(m_heightTexture, GL_R16F, GL_RED);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_heightTexture, 0);

        glGenTextures(1, &m_normalTexture);
        configureCacheTexture(m_normalTexture, GL_RG16F, GL_RG);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, m_normalTexture, 0);

        const GLenum drawBuffers[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
        glDrawBuffers(2, drawBuffers);

        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        {
            labhelper::fatal_error("Failed to create the terrain cache framebuffer.");
        }

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    /**
     * @brief Computes the world-space origin of the cached terrain window.
     * @param originGrid Grid coordinate of the cached terrain window's minimum corner.
     * @param tileWidth Width of one terrain tile in world units.
     * @return The world-space origin used when sampling the terrain cache.
     */
    glm::vec3 cacheOriginWorld(const glm::ivec2& originGrid, const float tileWidth) const
    {
        return glm::vec3(
            float(originGrid.x) * tileWidth,
            0.0f,
            float(originGrid.y) * tileWidth
        );
    }

    /**
     * @brief Computes the spacing between cached terrain samples in world space.
     * @param tileWidth Width of one terrain tile in world units.
     * @return The world-space distance between adjacent cached samples.
     */
    float cacheVertexSpacing(const float tileWidth) const
    {
        if (m_gridResolution <= 0)
        {
            labhelper::fatal_error("TerrainTileMesh::create must be called before cacheVertexSpacing.");
        }

        return tileWidth / float(m_gridResolution);
    }

    /**
     * @brief Checks whether the terrain cache must be regenerated.
     * @param originGrid Grid coordinate of the cached terrain window's minimum corner.
     * @param heightScale User-controlled terrain height multiplier.
     * @param tileWidth Width of one terrain tile in world units.
     * @param tileHeight Maximum terrain height in world units.
     * @param tileSeed Noise seed used by the terrain generator.
     * @return `true` if the cache contents no longer match the requested parameters.
     */
    bool cacheNeedsUpdate(const glm::ivec2& originGrid,
                          const float heightScale,
                          const float tileWidth,
                          const float tileHeight,
                          const int tileSeed) const
    {
        return !m_cacheState.valid
            || m_cacheState.originGrid != originGrid
            || m_cacheState.heightScale != heightScale
            || m_cacheState.tileWidth != tileWidth
            || m_cacheState.tileHeight != tileHeight
            || m_cacheState.tileSeed != tileSeed;
    }

    /**
     * @brief Regenerates the GPU terrain cache using the supplied terrain shader program.
     * @param terrainCacheProgram Shader program that renders the cache textures.
     * @param originGrid Grid coordinate of the cached terrain window's minimum corner.
     * @param heightScale User-controlled terrain height multiplier.
     * @param tileWidth Width of one terrain tile in world units.
     * @param tileHeight Maximum terrain height in world units.
     * @param tileSeed Noise seed used by the terrain generator.
     */
    void updateCache(const GLuint terrainCacheProgram,
                     const glm::ivec2& originGrid,
                     const float heightScale,
                     const float tileWidth,
                     const float tileHeight,
                     const int tileSeed)
    {
        if (m_cacheFbo == 0)
        {
            labhelper::fatal_error("TerrainTileMesh::createCache must be called before updateCache.");
        }

        const glm::vec3 terrainCacheOriginWorld = cacheOriginWorld(originGrid, tileWidth);
        const float terrainVertexSpacing = cacheVertexSpacing(tileWidth);
        const GLenum drawBuffers[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };

        glBindFramebuffer(GL_FRAMEBUFFER, m_cacheFbo);
        glDrawBuffers(2, drawBuffers);
        glViewport(0, 0, m_cacheResolution, m_cacheResolution);
        glClear(GL_COLOR_BUFFER_BIT);

        glUseProgram(terrainCacheProgram);
        labhelper::setUniformSlow(terrainCacheProgram, "terrainCacheOriginWorld", terrainCacheOriginWorld);
        labhelper::setUniformSlow(terrainCacheProgram, "terrainVertexSpacing", terrainVertexSpacing);
        labhelper::setUniformSlow(terrainCacheProgram, "heightScale", heightScale);
        labhelper::setUniformSlow(terrainCacheProgram, "tileHeight", tileHeight);
        labhelper::setUniformSlow(terrainCacheProgram, "tileSeed", tileSeed);

        labhelper::drawFullScreenQuad();
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        m_cacheState.valid = true;
        m_cacheState.originGrid = originGrid;
        m_cacheState.heightScale = heightScale;
        m_cacheState.tileWidth = tileWidth;
        m_cacheState.tileHeight = tileHeight;
        m_cacheState.tileSeed = tileSeed;
    }

    /**
     * @brief Binds the tile VAO so it can be drawn by the active shader.
     */
    void bind() const
    {
        glBindVertexArray(m_vao);
    }

    /**
     * @brief Draws the currently bound terrain tile mesh.
     */
    void draw() const
    {
        glDrawElements(GL_TRIANGLES, m_indexCount, GL_UNSIGNED_INT, nullptr);
    }

    /**
     * @brief Returns the cached terrain height texture.
     * @return OpenGL texture handle containing cached terrain heights.
     */
    GLuint heightTexture() const
    {
        return m_heightTexture;
    }

    /**
     * @brief Returns the cached terrain normal texture.
     * @return OpenGL texture handle containing cached terrain normals.
     */
    GLuint normalTexture() const
    {
        return m_normalTexture;
    }

    /**
     * @brief Returns the terrain cache resolution.
     * @return Number of cached samples along one texture axis.
     */
    int cacheResolution() const
    {
        return m_cacheResolution;
    }

private:
    /**
     * @brief Tracks which terrain parameters the current cache represents.
     */
    struct CacheState
    {
        bool valid = false;
        glm::ivec2 originGrid = glm::ivec2(0);
        float heightScale = 0.0f;
        float tileWidth = 0.0f;
        float tileHeight = 0.0f;
        int tileSeed = 0;
    };

    /**
     * @brief Vertex format uploaded for the reusable terrain tile.
     */
    struct Vertex
    {
        glm::vec3 position;
        glm::vec3 normal;
        glm::vec2 texcoord;
    };

    /**
     * @brief Allocates and configures one terrain cache texture.
     * @param texture OpenGL texture handle to initialize.
     * @param internalFormat Texture storage format used on the GPU.
     * @param format Base upload format matching the texture storage.
     */
    void configureCacheTexture(const GLuint texture, const GLenum internalFormat, const GLenum format) const
    {
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexImage2D(
            GL_TEXTURE_2D,
            0,
            internalFormat,
            m_cacheResolution,
            m_cacheResolution,
            0,
            format,
            GL_HALF_FLOAT,
            nullptr
        );
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    }

    GLuint m_vao = 0;
    GLuint m_vbo = 0;
    GLuint m_ebo = 0;
    GLsizei m_indexCount = 0;
    int m_gridResolution = 0;
    int m_cacheResolution = 0;
    GLuint m_heightTexture = 0;
    GLuint m_normalTexture = 0;
    GLuint m_cacheFbo = 0;
    CacheState m_cacheState;
};

class GUISettings
{
public:
    float lightIntensity = 5.3f;
    
    float heightMapScale = 1.0f;
    
    float terrainTextureScale = 0.1f;
    float blend = 0.02f;
    
    float cameraMoveSpeed = 250.0f;
    
    bool wireframeMode = false;
    
    bool uploadGuard = false;

    bool shadowsEnabled = false;
    int shadowMapSize = 4096;
    ShadowMap *shadowMap = nullptr; // Pointer to the shadow map, need this to update the shadow map when the size is changed

    float sunTimeOfDayAngle = 0.0f;
    float sunDistance    = 1200.0f;       // how far from the camera the sun is drawn
    float sunRadiusWorld = 70.0f;         // size of the visible sun in world units

    float atmosphereExposure = 1.15f;
    float atmosphereTurbidity = 2.2f;
    float atmosphereRayleigh = 2.4f;
    float atmosphereMieCoefficient = 0.0045f;
    float atmosphereMieDirectionalG = 0.82f;
    float atmosphereStarIntensity = 0.25f;
    float skyAmbientStrength = 0.18f;
    float aerialPerspectiveDensity = 0.0011f;

    std::vector<Material*> materials = { &waterMaterial, &sandMaterial, &grassMaterial, &rockMaterial, &snowMaterial };

    Material waterMaterial{
        glm::vec3(0.0f, 0.3f, 0.5f), // color
        0.964f, // metalness
        0.00f, // fresnel
        256.0f, // shininess
        glm::vec3(0.1f, 0.1f, 0.2f) // emission
    };
    Material sandMaterial{
        glm::vec3(0.76f, 0.70f, 0.50f), // color
        0.516f, // metalness
        0.013f, // fresnel
        43.202f, // shininess
        glm::vec3(0.0f) // emission
    };
    Material grassMaterial{
        glm::vec3(0.1f, 0.6f, 0.1f), // color
        0.0f, // metalness
        0.0f, // fresnel
        0.0f, // shininess
        glm::vec3(0.0f) // emission
    };
    Material rockMaterial{
        glm::vec3(0.5f, 0.5f, 0.5f), // color
        0.0f, // metalness
        0.0f, // fresnel
        0.0f, // shininess
        glm::vec3(0.0f) // emission
    };
    Material snowMaterial{
        glm::vec3(0.9f, 0.9f, 0.9f), // color
        0.0f, // metalness
        0.369f, // fresnel
        0.0f, // shininess
        glm::vec3(0.0f) // emission
    };

    GUISettings() = default;

    void sendToShader(const GLuint& terrainShader) const
    {
        waterMaterial.setMaterialUniform(terrainShader, "waterMaterial");
        sandMaterial.setMaterialUniform(terrainShader, "sandMaterial");
        grassMaterial.setMaterialUniform(terrainShader, "grassMaterial");
        rockMaterial.setMaterialUniform(terrainShader, "rockMaterial");
        snowMaterial.setMaterialUniform(terrainShader, "snowMaterial");
    }

};
