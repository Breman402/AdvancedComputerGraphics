#pragma once
#include "scatter.hpp"
#include <vector>

// Scatter Objects
ScatterObject treeScatterObject(
    "Tree", // Name
    {0.5f, 2.0f}, // Scale
    0.0f, // Offset
    {TerrainType::gras, TerrainType::gras}, // Terrain range
    15.0f, // Grid size
    100, // Frequency
    "../Adv-project/models/Tree/Tree.obj" // .obj path
); 

ScatterObject rockScatterObject(
    "Rock", // Name
    {0.2f, 3.0f}, // Scale
    0.0f, // Offset
    {TerrainType::gras, TerrainType::rock}, // Terrain range
    50.0f, // Grid size
    100, // Frequency
    "../Adv-project/models/Rock1/Rock1.obj" // .obj path
);

ScatterObject snowRockScatterObject(
    "SnowRock", // Name
    {0.6f, 1.0f}, // Scale
    0.0f, // Offset
    {TerrainType::rock, TerrainType::snow}, // Terrain range
    50.0f, // Grid size
    100, // Frequency
    "../Adv-project/models/SnowRock/quaternius_cc0-snowy-rock-1310.obj" // .obj path, using the same rock model as the regular rock scatter object since we will just use a different material for it in the shader
);

std::vector<ScatterObject*> scatterObjects = { &treeScatterObject, &rockScatterObject, &snowRockScatterObject };