#pragma once

#include <GL/glew.h>
#include <glm/glm.hpp>
#include <imgui.h>
#include <SDL2/SDL.h>

struct SunLightingState
{
    glm::vec3 sunDirection = glm::vec3(0.0f, 1.0f, 0.0f);
    glm::vec3 directLightColor = glm::vec3(1.0f); 
    glm::vec3 discColor = glm::vec3(1.0f); // Updated by evaluateSunLighting, represents the color of the sun's disc, which is the core part of the sun that is visible when looking directly at it. This color is affected by atmospheric scattering and changes based on the sun's height in the sky.
    glm::vec3 haloColor = glm::vec3(1.0f); // Updated by evaluateSunLighting, represents the color of the sun's halo, which is the glow around the sun that is visible even when not looking directly at it. This color is typically a softer and more diffuse version of the disc color, and it also changes based on the sun's height in the sky.
    glm::vec3 skyAmbientColor = glm::vec3(0.2f, 0.3f, 0.4f); // Updated by evaluateSunLighting, 
    glm::vec3 fogColor = glm::vec3(0.2f, 0.3f, 0.4f);
    float sunVisibility = 1.0f;
};

void syncLightDirectionFromSunCycle(glm::vec3& lightDirWorld, float& sunTimeOfDayAngle);

void syncSunCycleFromLightDirection(glm::vec3& lightDirWorld, float& sunTimeOfDayAngle);

void updateSunFromKeyboard(float deltaTimeSeconds, glm::vec3& lightDirWorld, float& sunTimeOfDayAngle);

SunLightingState evaluateSunLighting(const glm::vec3& lightDirWorld, float lightIntensity);
