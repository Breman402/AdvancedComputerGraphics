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

/**
 * @brief Derives the scene's sky, sun, and fog lighting from the current sun direction.
 *
 * This helper converts the directional light vector into a higher-level lighting state used
 * by the terrain, atmosphere, and visible sun passes. It estimates how high the sun is above
 * the horizon, then uses that to blend between night, twilight, and daylight color palettes.
 * The function also applies a simple atmospheric transmittance approximation so the sun becomes
 * warmer and dimmer near the horizon, while noon lighting stays whiter and stronger.
 *
 * @param lightDirWorld Normalized world-space light direction pointing from the sun toward the scene.
 * @param lightIntensity User-controlled scalar for the overall brightness of the directional light.
 * @return SunLightingState Aggregated lighting colors and visibility terms shared by the render passes.
 */
SunLightingState evaluateSunLighting(const glm::vec3& lightDirWorld, float lightIntensity)
{
    SunLightingState state;
    state.sunDirection = glm::normalize(-lightDirWorld);

    const float sunHeight = glm::clamp(state.sunDirection.y, -1.0f, 1.0f);
    const float daylight = glm::smoothstep(-0.08f, 0.14f, sunHeight);
    const float twilight = (1.0f - daylight) * glm::smoothstep(-0.22f, 0.05f, sunHeight);
    const float airMass = 1.0f / glm::max(0.08f, sunHeight + 0.18f);
    const float opticalPath = glm::max(0.0f, airMass - 1.0f);

    const glm::vec3 warmSun(1.0f, 0.58f, 0.30f);
    const glm::vec3 whiteSun(1.0f, 0.97f, 0.92f);
    const glm::vec3 transmittance = glm::exp(-glm::vec3(0.18f, 0.40f, 0.90f) * opticalPath * 0.65f);
    const float sunColorMix = glm::smoothstep(-0.02f, 0.45f, sunHeight);
    const float lightScale = glm::clamp(lightIntensity / 5.3f, 0.35f, 2.0f);

    state.discColor = glm::mix(warmSun, whiteSun, sunColorMix) * transmittance;
    state.haloColor = glm::mix(state.discColor, glm::vec3(1.0f, 0.72f, 0.45f), 0.35f);
    state.sunVisibility = glm::smoothstep(-0.18f, -0.02f, sunHeight);

    const glm::vec3 nightAmbient(0.015f, 0.020f, 0.040f); // Subjective guessed values that are never updated
    const glm::vec3 dayAmbient(0.32f, 0.44f, 0.62f);
    const glm::vec3 twilightAmbient(0.46f, 0.25f, 0.10f);

    state.skyAmbientColor = glm::mix(nightAmbient, dayAmbient, daylight);
    state.skyAmbientColor = glm::mix(state.skyAmbientColor, twilightAmbient, twilight * 0.6f);
    state.skyAmbientColor *= lightScale;

    state.fogColor = glm::mix(state.skyAmbientColor, state.haloColor, 0.18f + twilight * 0.25f);
    state.directLightColor = glm::mix(glm::vec3(0.08f, 0.10f, 0.15f), state.discColor, daylight + twilight * 0.5f);

    return state;
}
