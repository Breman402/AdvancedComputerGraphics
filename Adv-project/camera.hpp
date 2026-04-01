#pragma once

#include <SDL2/SDL.h>
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>


// Mouse rotation state
extern bool leftMouseDown;
extern bool rightMouseDown;
extern bool middleMouseDown;
extern int  lastMouseX;
extern int  lastMouseY;

// Camera rotation & controlls
extern float cameraYaw;
extern float cameraPitch;
extern float viewYawOffset;
extern float viewPitchOffset;
extern glm::vec3 cameraPosition; // This will be the starting pos of camera in worldspace

void handleMouseMotion(const SDL_Event& event);

void handleSDLEvents(const SDL_Event& event, bool& stopRendering);
void updateCamera(float deltaTimeSeconds, float moveSpeedUnitsPerSecond);
glm::vec3 getCameraForward();
glm::vec3 getCameraRight();
