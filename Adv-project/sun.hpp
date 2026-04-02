#pragma once

#include <GL/glew.h>
#include <glm/glm.hpp>
#include <imgui.h>
#include <SDL2/SDL.h>

void syncLightDirectionFromSunCycle(glm::vec3& lightDirWorld, float& sunTimeOfDayAngle);

void syncSunCycleFromLightDirection(glm::vec3& lightDirWorld, float& sunTimeOfDayAngle);

void updateSunFromKeyboard(float deltaTimeSeconds, glm::vec3& lightDirWorld, float& sunTimeOfDayAngle);
