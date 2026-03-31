#pragma once
#include "sphereMesh.hpp"
#include "createShadow.hpp"
#include <glm/glm.hpp>

struct Material
{
    glm::vec3 color;
    float metalness;
    float fresnel;
    float shininess;
    glm::vec3 emission;
};

struct EarthTextureToUse
{
    bool useBlueMarbel8kPNG = false;
    bool useMapMode = false;
    bool useGrayScaleHeightMap = false;
};

void gui(SphereMesh& sphere, float& heightMapScale, int& shadowMapSize, ShadowMap& shadowMap, Material& earthMaterial, bool& wireframeMode, bool& showTexture, EarthTextureToUse& earthTextureOption);