#pragma once

#include <GL/glew.h>
#include <glm/glm.hpp>

#include "classes.hpp"
#include "sun.hpp"

struct ShaderTextureUnits
{
    int terrainWater = 0;
    int terrainSand = 1;
    int terrainGrass0 = 2;
    int terrainGrass1 = 3;
    int terrainGrass2 = 4;
    int terrainGrass3 = 5;
    int terrainRock0 = 6;
    int terrainRock1 = 7;
    int terrainRock2 = 8;
    int terrainRock3 = 9;
    int terrainSnow0 = 10;
    int terrainSnow1 = 11;
    int terrainSnow2 = 12;
    int terrainSnow3 = 13;
    int terrainShadow = 14;
    int terrainHeight = 15;
    int terrainNormal = 16;
    int starField = 17;
};

struct ShaderTextureHandles
{
    GLuint water = 0;
    GLuint sand = 0;
    GLuint grass[4] = {};
    GLuint rock[4] = {};
    GLuint snow[4] = {};
    GLuint starField = 0;
};

class ShaderUniforms
{
public:
    ShaderTextureUnits textureUnits;

    void track(const GUISettings& trackedSettings,
               const glm::mat4& trackedLightViewMatrix,
               const glm::mat4& trackedLightProjectionMatrix,
               const glm::vec3& trackedLightDirWorld,
               const glm::vec3& trackedCameraPosWorld,
               const SunLightingState& trackedSunLighting,
               const ShadowMap& trackedShadowMap)
    {
        settings = &trackedSettings;
        lightViewMatrix = &trackedLightViewMatrix;
        lightProjectionMatrix = &trackedLightProjectionMatrix;
        lightDirWorld = &trackedLightDirWorld;
        cameraPosWorld = &trackedCameraPosWorld;
        sunLighting = &trackedSunLighting;
        shadowMap = &trackedShadowMap;
    }

    void setTerrainCache(const glm::vec3& cacheOriginWorld,
                         const float vertexSpacing,
                         const int cacheResolution,
                         const float height,
                         const GLuint heightTexture,
                         const GLuint normalTexture)
    {
        terrainCacheOriginWorld = cacheOriginWorld;
        terrainVertexSpacing = vertexSpacing;
        terrainCacheResolution = cacheResolution;
        tileHeight = height;
        terrainHeightTexture = heightTexture;
        terrainNormalTexture = normalTexture;
    }

    glm::mat4 lightViewProj() const
    {
        return (*lightProjectionMatrix) * (*lightViewMatrix);
    }

    void bindTerrainMaterialTextures(const ShaderTextureHandles& textures) const
    {
        bindTexture(textureUnits.terrainWater, textures.water);
        bindTexture(textureUnits.terrainSand, textures.sand);
        bindTexture(textureUnits.terrainGrass0, textures.grass[0]);
        bindTexture(textureUnits.terrainGrass1, textures.grass[1]);
        bindTexture(textureUnits.terrainGrass2, textures.grass[2]);
        bindTexture(textureUnits.terrainGrass3, textures.grass[3]);
        bindTexture(textureUnits.terrainRock0, textures.rock[0]);
        bindTexture(textureUnits.terrainRock1, textures.rock[1]);
        bindTexture(textureUnits.terrainRock2, textures.rock[2]);
        bindTexture(textureUnits.terrainRock3, textures.rock[3]);
        bindTexture(textureUnits.terrainSnow0, textures.snow[0]);
        bindTexture(textureUnits.terrainSnow1, textures.snow[1]);
        bindTexture(textureUnits.terrainSnow2, textures.snow[2]);
        bindTexture(textureUnits.terrainSnow3, textures.snow[3]);
    }

    void bindTerrainHeightTexture() const
    {
        bindTexture(textureUnits.terrainHeight, terrainHeightTexture);
    }

    void bindTerrainCacheTextures() const
    {
        bindTexture(textureUnits.terrainHeight, terrainHeightTexture);
        bindTexture(textureUnits.terrainNormal, terrainNormalTexture);
    }

    void bindShadowTexture() const
    {
        bindTexture(textureUnits.terrainShadow, shadowMap->depthTex);
    }

    void bindStarTexture(const GLuint starTexture) const
    {
        bindTexture(textureUnits.starField, starTexture);
    }

    void uploadShadowTerrain(const GLuint shaderProgram) const
    {
        labhelper::setUniformSlow(shaderProgram, "lightViewProj", lightViewProj());
        uploadTerrainCacheUniforms(shaderProgram);
        labhelper::setUniformSlow(shaderProgram, "terrainHeightTex", textureUnits.terrainHeight);
    }

    void uploadTerrainStatic(const GLuint shaderProgram) const
    {
        labhelper::setUniformSlow(shaderProgram, "terrainWaterTex", textureUnits.terrainWater);
        labhelper::setUniformSlow(shaderProgram, "terrainSandTex", textureUnits.terrainSand);
        labhelper::setUniformSlow(shaderProgram, "terrainGrassTex0", textureUnits.terrainGrass0);
        labhelper::setUniformSlow(shaderProgram, "terrainGrassTex1", textureUnits.terrainGrass1);
        labhelper::setUniformSlow(shaderProgram, "terrainGrassTex2", textureUnits.terrainGrass2);
        labhelper::setUniformSlow(shaderProgram, "terrainGrassTex3", textureUnits.terrainGrass3);
        labhelper::setUniformSlow(shaderProgram, "terrainRockTex0", textureUnits.terrainRock0);
        labhelper::setUniformSlow(shaderProgram, "terrainRockTex1", textureUnits.terrainRock1);
        labhelper::setUniformSlow(shaderProgram, "terrainRockTex2", textureUnits.terrainRock2);
        labhelper::setUniformSlow(shaderProgram, "terrainRockTex3", textureUnits.terrainRock3);
        labhelper::setUniformSlow(shaderProgram, "terrainSnowTex0", textureUnits.terrainSnow0);
        labhelper::setUniformSlow(shaderProgram, "terrainSnowTex1", textureUnits.terrainSnow1);
        labhelper::setUniformSlow(shaderProgram, "terrainSnowTex2", textureUnits.terrainSnow2);
        labhelper::setUniformSlow(shaderProgram, "terrainSnowTex3", textureUnits.terrainSnow3);
        labhelper::setUniformSlow(shaderProgram, "terrainHeightTex", textureUnits.terrainHeight);
        labhelper::setUniformSlow(shaderProgram, "terrainNormalTex", textureUnits.terrainNormal);
        labhelper::setUniformSlow(shaderProgram, "shadowMap", textureUnits.terrainShadow);

        settings->sendToShader(shaderProgram);
        labhelper::setUniformSlow(shaderProgram, "shadowsEnabled", settings->shadowsEnabled);
        labhelper::setUniformSlow(shaderProgram, "point_light_intensity_multiplier", settings->lightIntensity);
        labhelper::setUniformSlow(shaderProgram, "terrainTextureScale", settings->terrainTextureScale);
        labhelper::setUniformSlow(shaderProgram, "blend", settings->blend);
        labhelper::setUniformSlow(shaderProgram, "wireframeMode", settings->wireframeMode);
        labhelper::setUniformSlow(shaderProgram, "showTexture", 1);
        labhelper::setUniformSlow(shaderProgram, "tileHeight", tileHeight);
    }

    void uploadTerrainFrame(const GLuint shaderProgram) const
    {
        labhelper::setUniformSlow(shaderProgram, "lightViewProj", lightViewProj());
        labhelper::setUniformSlow(shaderProgram, "lightDirWorld", *lightDirWorld);
        labhelper::setUniformSlow(shaderProgram, "cameraPosWorld", *cameraPosWorld);
        labhelper::setUniformSlow(shaderProgram, "point_light_color", sunLighting->directLightColor);
        labhelper::setUniformSlow(shaderProgram, "skyAmbientColor", sunLighting->skyAmbientColor);
        labhelper::setUniformSlow(shaderProgram, "skyAmbientStrength", settings->skyAmbientStrength);
        labhelper::setUniformSlow(shaderProgram, "atmosphereFogColor", sunLighting->fogColor);
        labhelper::setUniformSlow(shaderProgram, "atmosphereFogDensity", settings->aerialPerspectiveDensity);
        uploadTerrainCacheUniforms(shaderProgram);
    }

    void uploadScatterTerrain(const GLuint shaderProgram) const
    {
        labhelper::setUniformSlow(shaderProgram, "terrainHeightTex", textureUnits.terrainHeight);
        uploadTerrainCacheUniforms(shaderProgram);
        labhelper::setUniformSlow(shaderProgram, "tileHeight", tileHeight);
        labhelper::setUniformSlow(shaderProgram, "color_texture", 0);
    }

    void uploadAtmosphere(const GLuint shaderProgram, const float sunIntensityScale) const
    {
        labhelper::setUniformSlow(shaderProgram, "sunDirectionWorld", sunLighting->sunDirection);
        labhelper::setUniformSlow(shaderProgram, "turbidity", settings->atmosphereTurbidity);
        labhelper::setUniformSlow(shaderProgram, "rayleigh", settings->atmosphereRayleigh);
        labhelper::setUniformSlow(shaderProgram, "mieCoefficient", settings->atmosphereMieCoefficient);
        labhelper::setUniformSlow(shaderProgram, "mieDirectionalG", settings->atmosphereMieDirectionalG);
        labhelper::setUniformSlow(shaderProgram, "sunIntensityScale", sunIntensityScale);
        labhelper::setUniformSlow(shaderProgram, "exposure", settings->atmosphereExposure);
        labhelper::setUniformSlow(shaderProgram, "starIntensity", settings->atmosphereStarIntensity);
        labhelper::setUniformSlow(shaderProgram, "starTexture", textureUnits.starField);
    }

    void uploadSun(const GLuint shaderProgram,
                   const glm::mat4& mvpMatrix,
                   const glm::mat4& modelMatrix,
                   const float sunIntensityScale) const
    {
        labhelper::setUniformSlow(shaderProgram, "mvpMatrix", mvpMatrix);
        labhelper::setUniformSlow(shaderProgram, "modelMatrix", modelMatrix);
        labhelper::setUniformSlow(shaderProgram, "cameraPosWorld", *cameraPosWorld);
        labhelper::setUniformSlow(shaderProgram, "sunCoreColor", sunLighting->discColor);
        labhelper::setUniformSlow(shaderProgram, "sunHaloColor", sunLighting->haloColor);
        labhelper::setUniformSlow(shaderProgram, "sunIntensityScale", sunIntensityScale);
    }

private:
    static void bindTexture(const int textureUnit, const GLuint texture)
    {
        glActiveTexture(GL_TEXTURE0 + textureUnit);
        glBindTexture(GL_TEXTURE_2D, texture);
    }

    void uploadTerrainCacheUniforms(const GLuint shaderProgram) const
    {
        labhelper::setUniformSlow(shaderProgram, "terrainCacheOriginWorld", terrainCacheOriginWorld);
        labhelper::setUniformSlow(shaderProgram, "terrainVertexSpacing", terrainVertexSpacing);
        labhelper::setUniformSlow(shaderProgram, "terrainCacheResolution", terrainCacheResolution);
    }

    const GUISettings* settings = nullptr;
    const glm::mat4* lightViewMatrix = nullptr;
    const glm::mat4* lightProjectionMatrix = nullptr;
    const glm::vec3* lightDirWorld = nullptr;
    const glm::vec3* cameraPosWorld = nullptr;
    const SunLightingState* sunLighting = nullptr;
    const ShadowMap* shadowMap = nullptr;

    glm::vec3 terrainCacheOriginWorld = glm::vec3(0.0f);
    float terrainVertexSpacing = 1.0f;
    int terrainCacheResolution = 1;
    float tileHeight = 1.0f;
    GLuint terrainHeightTexture = 0;
    GLuint terrainNormalTexture = 0;
};
