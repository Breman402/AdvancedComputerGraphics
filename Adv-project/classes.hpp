#pragma once

#include <glm/glm.hpp>
#include <labhelper.h>
#include <string>
#include <vector>

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
    float lightIntensity = 8.2f;
    float heightMapScale = 1.0f;
    float terrainTextureScale = 0.1f;
    float blend = 0.02f;
    float cameraMoveSpeed = 250.0f;
    bool wireframeMode = false;
    bool uploadGuard = false;
    std::vector<Material*> materials;

    GUISettings() = default;
    explicit GUISettings(const std::vector<Material*>& initialMaterials)
        : materials(initialMaterials) {}

};
