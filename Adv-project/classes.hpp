#pragma once

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
        32.0f, // shininess
        glm::vec3(0.0f) // emission
    };
    Material rockMaterial{
        glm::vec3(0.5f, 0.5f, 0.5f), // color
        0.0f, // metalness
        0.0f, // fresnel
        8.24f, // shininess
        glm::vec3(0.0f) // emission
    };
    Material snowMaterial{
        glm::vec3(0.9f, 0.9f, 0.9f), // color
        0.0f, // metalness
        0.369f, // fresnel
        1.0f, // shininess
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
