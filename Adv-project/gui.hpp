#pragma once
#include <glm/glm.hpp>
#include <vector>

#include "classes.hpp"

void gui(float* lightIntensity, float* heightMapScale, float* terrainTextureScale, float* blend, float* cameraMoveSpeed, std::vector<Material*>* materials, bool* wireframeMode, bool* uploadGuard);
