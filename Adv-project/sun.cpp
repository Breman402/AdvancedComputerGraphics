#include "sun.hpp"

// Globals
glm::vec3 sunOrbitForward = glm::vec3(0.0f, 0.0f, -1.0f);

void syncLightDirectionFromSunCycle(glm::vec3& lightDirWorld, float& sunTimeOfDayAngle)
{
    const glm::vec3 worldUp(0.0f, 1.0f, 0.0f);
    const glm::vec3 sunDirection = normalize(glm::vec3(
        sunOrbitForward * cos(sunTimeOfDayAngle) +
        worldUp * sin(sunTimeOfDayAngle)
    ));

    lightDirWorld = -sunDirection;
}

void syncSunCycleFromLightDirection(glm::vec3& lightDirWorld, float& sunTimeOfDayAngle)
{
    const glm::vec3 worldUp(0.0f, 1.0f, 0.0f);
    const glm::vec3 sunDirection = normalize(-lightDirWorld);
    const glm::vec3 horizontalDirection = sunDirection - dot(sunDirection, worldUp) * worldUp;

    if (length(horizontalDirection) > 1e-5f)
    {
        sunOrbitForward = normalize(horizontalDirection);
    }

    sunTimeOfDayAngle = std::atan2(sunDirection.y, dot(sunDirection, sunOrbitForward));
    syncLightDirectionFromSunCycle(lightDirWorld, sunTimeOfDayAngle);
}

void updateSunFromKeyboard(float deltaTimeSeconds, glm::vec3& lightDirWorld, float& sunTimeOfDayAngle)
{
    constexpr float kSunCycleSpeed = glm::radians(30.0f);
    ImGuiIO& io = ImGui::GetIO();
    if (io.WantCaptureKeyboard)
    {
        return;
    }

    const Uint8* keyboardState = SDL_GetKeyboardState(nullptr);
    const float angleStep = kSunCycleSpeed * deltaTimeSeconds;

    if (keyboardState[SDL_SCANCODE_LEFT])
    {
        sunTimeOfDayAngle -= angleStep;
    }
    if (keyboardState[SDL_SCANCODE_RIGHT])
    {
        sunTimeOfDayAngle += angleStep;
    }

    syncLightDirectionFromSunCycle(lightDirWorld, sunTimeOfDayAngle);
}