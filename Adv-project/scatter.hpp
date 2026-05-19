#pragma once
#include <string>
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/common.hpp>
#include <GL/glew.h>
#include <cmath>
#include <climits>
#include <utility>
#include <vector>
#include <labhelper.h>
#include <Model.h>
#include "shaderUniforms.hpp"

enum class TerrainType
{
    water,
    sand,
    gras,
    rock,
    snow
};

using namespace std;

struct ScatterInstance
{
    glm::vec2 worldPos;
    float scale;
    float rotation;
};

class ScatterObject
{
    public:
  
    ScatterObject(const string& name, pair<float, float> scale, float offset, pair<TerrainType, TerrainType> terrainRange, float grid, int freq, const string& modelPath){
        this->name = name;
        this->scale = scale;
        this->offset = offset;
        this->terrainRange = terrainRange;
        this->grid = grid;
        this->freq = freq;
        this->modelPath = modelPath;
    }

    void updateInstances(int centerGridX, int centerGridZ, int renderDistance, float terrainSquareSize){
        instances = generateInstances(centerGridX, centerGridZ, renderDistance, terrainSquareSize);
    }

    const vector<ScatterInstance>& getInstances() const{
        return instances;
    }

    // This is in case I want to update some of the scatter object parameters in runtime later
    void changeScatterObjectSettings(pair<float, float> newScale, float newOffset, float newgrid, int newfreq){
        this->scale = newScale;
        this->offset = newOffset;
        this->grid = newgrid;
        this->freq = newfreq;
    }

    labhelper::Model* getModel(){
        return model;
    }

    const pair<TerrainType, TerrainType>& getTerrainRange() const{
        return terrainRange;
    }

    float getOffset() const{
        return offset;
    }

    void loadModel(){
        if (model == nullptr)
        {
            model = labhelper::loadModelFromOBJ(modelPath);
        }
    }

    vector<ScatterInstance> generateInstances(int centerGridX, int centerGridZ, int renderDistance, float terrainSquareSize){
        vector<ScatterInstance> instances;

        const float minWorldX = float(centerGridX - renderDistance) * terrainSquareSize;
        const float maxWorldX = float(centerGridX + renderDistance + 1) * terrainSquareSize;
        const float minWorldZ = float(centerGridZ - renderDistance) * terrainSquareSize;
        const float maxWorldZ = float(centerGridZ + renderDistance + 1) * terrainSquareSize;

        const int minCellX = int(std::floor(minWorldX / grid));
        const int maxCellX = int(std::floor(maxWorldX / grid));
        const int minCellZ = int(std::floor(minWorldZ / grid));
        const int maxCellZ = int(std::floor(maxWorldZ / grid));

        for (int cellZ = minCellZ; cellZ < maxCellZ; ++cellZ)
        {
            for (int cellX = minCellX; cellX < maxCellX; ++cellX)
            {
                const float spawnChance = random01(cellX, cellZ, RandomValue::SpawnChance);
                if (spawnChance > spawnFrequency())
                {
                    continue;
                }

                // Pick a stable random point inside this scatter cell, then convert it to world XZ.
                const float localX = random01(cellX, cellZ, RandomValue::LocalX) * grid;
                const float localZ = random01(cellX, cellZ, RandomValue::LocalZ) * grid;
                const float worldX = float(cellX) * grid + localX;
                const float worldZ = float(cellZ) * grid + localZ;

                const float scaleT = random01(cellX, cellZ, RandomValue::Scale);
                const float instanceScale = glm::mix(scale.first, scale.second, scaleT);
                const float instanceRotation = random01(cellX, cellZ, RandomValue::Rotation) * glm::two_pi<float>();

                instances.emplace_back(ScatterInstance{
                    glm::vec2(worldX, worldZ),
                    instanceScale,
                    instanceRotation
                });

            }
        }

        return instances;

    }

    private:

    enum class RandomValue
    {
        SpawnChance,
        LocalX,
        LocalZ,
        Scale,
        Rotation
    };

    int hashCoords(const int cellX, const int cellZ, const RandomValue value) const
    {
        // Start from the object seed and cell coordinates so every cell is stable
        // across frames, camera movement, and regeneration.
        unsigned int h = unsigned(seed) ^ (unsigned(value) * 0x9e3779b9u);
        h ^= unsigned(cellX) * 0x85ebca6bu;
        h ^= unsigned(cellZ) * 0xc2b2ae35u;

        // Avalanche the bits so neighboring cells do not produce similar values.
        h ^= h >> 16;
        h *= 0x7feb352du;
        h ^= h >> 15;
        h *= 0x846ca68bu;
        h ^= h >> 16;
        return int(h & 0x7fffffffu);
    }

    float random01(const int cellX, const int cellZ, const RandomValue value) const
    {
        return float(hashCoords(cellX, cellZ, value)) / float(INT_MAX);
    }

    float spawnFrequency() const
    {
        return glm::clamp(float(freq) / 100.0f, 0.0f, 1.0f);
    }

    string name; // This is just for keeping track of the name of the scatterobj
    pair<float, float> scale; // Lower and upper bound scale
    float offset;
    string modelPath;
    int freq; // 100% freq per gridsize for a scatter obj to spawn in every gridsquare
    float grid; // The size of each grid cell
    pair<TerrainType, TerrainType> terrainRange; // The lowest and highest terrain that the scatter obj. will spawn at
    vector<ScatterInstance> instances;
    int seed = 1337; // This should be unique per scatter object type so different objects do not share identical placements.
    labhelper::Model* model = nullptr;
};

class ScatterObjectRenderer
{
    public:
    ScatterObjectRenderer(const vector<ScatterObject*>& scatterObjects,
                          const string& vertShaderPath,
                          const string& fragShaderPath){
        this->renderer = labhelper::loadShaderProgram(vertShaderPath, fragShaderPath);
        this->scatterObjects = scatterObjects;
        glGenBuffers(1, &instanceVbo);

        for (ScatterObject* scatterObject : scatterObjects)
        {
            scatterObject->loadModel();
        }
    }

    void render(
        const int cameraGridX,
        const int cameraGridZ,
        const int renderDistance,
        const float width,
        const glm::mat4& projection,
        const glm::mat4& view,
        const ShaderUniforms& shaderUniforms)
    {
        glUseProgram(renderer);

        shaderUniforms.bindTerrainHeightTexture();
        shaderUniforms.bindShadowTexture();

        labhelper::setUniformSlow(renderer, "viewProjectionMatrix", projection * view);
        shaderUniforms.uploadScatterTerrain(renderer);
        shaderUniforms.uploadScatterFrame(renderer);

        for (ScatterObject* scatterObject : scatterObjects)
        {
            scatterObject->updateInstances(cameraGridX, cameraGridZ, renderDistance, width);
            const vector<ScatterInstance>& objInstances = scatterObject->getInstances();
            const labhelper::Model* objectModel = scatterObject->getModel();
            if (objectModel == nullptr)
            {
                continue;
            }

            const pair<TerrainType, TerrainType>& terrainRange = scatterObject->getTerrainRange();
            float minTerrainHeight01 = terrainTypeLowerBound(terrainRange.first);
            float maxTerrainHeight01 = terrainTypeUpperBound(terrainRange.second);
            if (minTerrainHeight01 > maxTerrainHeight01)
            {
                const float temp = minTerrainHeight01;
                minTerrainHeight01 = terrainTypeLowerBound(terrainRange.second);
                maxTerrainHeight01 = terrainTypeUpperBound(terrainRange.first);
            }

            labhelper::setUniformSlow(renderer, "minTerrainHeight01", minTerrainHeight01);
            labhelper::setUniformSlow(renderer, "maxTerrainHeight01", maxTerrainHeight01);
            labhelper::setUniformSlow(renderer, "heightOffset", scatterObject->getOffset());

            for (const ScatterInstance& instance : objInstances)
            {
                const glm::vec3 instancePosition(instance.worldPos.x, 0.0f, instance.worldPos.y);
                const glm::mat4 modelMatrix =
                    glm::translate(glm::mat4(1.0f), instancePosition) *
                    glm::rotate(glm::mat4(1.0f), instance.rotation, glm::vec3(0.0f, 1.0f, 0.0f)) *
                    glm::scale(glm::mat4(1.0f), glm::vec3(instance.scale));

                labhelper::setUniformSlow(renderer, "modelMatrix", modelMatrix);

                labhelper::render(objectModel);
            }
        }

    }

    void renderShadow(
        const int cameraGridX,
        const int cameraGridZ,
        const int renderDistance,
        const float width,
        const ShaderUniforms& shaderUniforms,
        const GLuint shadowProgram)
    {
        glUseProgram(shadowProgram);

        shaderUniforms.bindTerrainHeightTexture();
        shaderUniforms.uploadScatterShadow(shadowProgram);

        for (ScatterObject* scatterObject : scatterObjects)
        {
            scatterObject->updateInstances(cameraGridX, cameraGridZ, renderDistance, width);
            const vector<ScatterInstance>& objInstances = scatterObject->getInstances();
            const labhelper::Model* objectModel = scatterObject->getModel();
            if (objectModel == nullptr)
            {
                continue;
            }

            const pair<TerrainType, TerrainType>& terrainRange = scatterObject->getTerrainRange();
            float minTerrainHeight01 = terrainTypeLowerBound(terrainRange.first);
            float maxTerrainHeight01 = terrainTypeUpperBound(terrainRange.second);
            if (minTerrainHeight01 > maxTerrainHeight01)
            {
                const float temp = minTerrainHeight01;
                minTerrainHeight01 = terrainTypeLowerBound(terrainRange.second);
                maxTerrainHeight01 = terrainTypeUpperBound(terrainRange.first);
            }

            labhelper::setUniformSlow(shadowProgram, "minTerrainHeight01", minTerrainHeight01);
            labhelper::setUniformSlow(shadowProgram, "maxTerrainHeight01", maxTerrainHeight01);
            labhelper::setUniformSlow(shadowProgram, "heightOffset", scatterObject->getOffset());

            for (const ScatterInstance& instance : objInstances)
            {
                const glm::vec3 instancePosition(instance.worldPos.x, 0.0f, instance.worldPos.y);
                const glm::mat4 modelMatrix =
                    glm::translate(glm::mat4(1.0f), instancePosition) *
                    glm::rotate(glm::mat4(1.0f), instance.rotation, glm::vec3(0.0f, 1.0f, 0.0f)) *
                    glm::scale(glm::mat4(1.0f), glm::vec3(instance.scale));

                labhelper::setUniformSlow(shadowProgram, "modelMatrix", modelMatrix);

                labhelper::render(objectModel);
            }
        }
    }
    
    private:
    static float terrainTypeLowerBound(const TerrainType terrainType)
    {
        switch (terrainType)
        {
            case TerrainType::water: return 0.0f;
            case TerrainType::sand:  return 0.03f;
            case TerrainType::gras:  return 0.06f;
            case TerrainType::rock:  return 0.25f;
            case TerrainType::snow:  return 0.50f;
        }
        return 0.0f;
    }

    static float terrainTypeUpperBound(const TerrainType terrainType)
    {
        switch (terrainType)
        {
            case TerrainType::water: return 0.03f;
            case TerrainType::sand:  return 0.06f;
            case TerrainType::gras:  return 0.25f;
            case TerrainType::rock:  return 0.50f;
            case TerrainType::snow:  return 1.0f;
        }
        return 1.0f;
    }

    GLuint renderer;
    GLuint instanceVbo;
    vector<ScatterObject*> scatterObjects;

};
