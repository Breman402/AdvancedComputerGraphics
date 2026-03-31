#pragma once

#include <glm/glm.hpp>
#include <labhelper.h>

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