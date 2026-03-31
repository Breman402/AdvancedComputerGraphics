#include "camera.hpp"

using namespace glm;

bool leftMouseDown = false;
bool rightMouseDown = false;
bool middleMouseDown = false;
int lastMouseX = 0;
int lastMouseY = 0;

float cameraYaw = 0.0f;
float cameraPitch = 0.0f;
float viewYawOffset = 0.0f;
float viewPitchOffset = 0.0f;
float cameraMoveSpeed = 250.0f;
glm::vec3 cameraPosition = glm::vec3(0.0f, 0.0f, 3.0f);

namespace
{
void computeCameraBasis(vec3& moveForward, vec3& moveRight, vec3& worldUp)
{
    worldUp = vec3(0.0f, 1.0f, 0.0f);
    const float totalYaw = cameraYaw + viewYawOffset;
    const float totalPitch = clamp(cameraPitch + viewPitchOffset, radians(-89.0f), radians(89.0f));

    moveForward = normalize(vec3(
        sin(totalYaw) * cos(totalPitch),
        sin(totalPitch),
        -cos(totalYaw) * cos(totalPitch)
    ));
    moveRight = cross(moveForward, worldUp);
    if (length(moveRight) < 1e-5f)
    {
        moveRight = vec3(1.0f, 0.0f, 0.0f);
    }
    moveRight = normalize(moveRight);
}
}

glm::vec3 getCameraForward()
{
    vec3 moveForward, moveRight, worldUp;
    computeCameraBasis(moveForward, moveRight, worldUp);
    return moveForward;
}

glm::vec3 getCameraRight()
{
    vec3 moveForward, moveRight, worldUp;
    computeCameraBasis(moveForward, moveRight, worldUp);
    return moveRight;
}

void handleMouseMotion(const SDL_Event& event)
{
    const int dx = event.motion.x - lastMouseX;
    const int dy = event.motion.y - lastMouseY;

    const float rotationSpeed = 0.01f;

    if (rightMouseDown)
    {
        // Orbit the camera around Earth
        cameraYaw   += dx * rotationSpeed;
        cameraPitch += dy * rotationSpeed;

        const float maxCamPitch = radians(89.0f);
        cameraPitch = clamp(cameraPitch, -maxCamPitch, maxCamPitch);
    }
    else if (middleMouseDown)
    {
        // Tilt / angle the camera view (without moving the camera)
        viewYawOffset   += dx * rotationSpeed;
        viewPitchOffset += dy * rotationSpeed;

        const float maxViewPitch = radians(80.0f);
        viewPitchOffset = clamp(viewPitchOffset, -maxViewPitch, maxViewPitch);
    }

    lastMouseX = event.motion.x;
    lastMouseY = event.motion.y;
}

void handleSDLEvents(const SDL_Event& event, bool& stopRendering)
{
	SDL_Keycode key;

    const float zoomSpeed = 40.0f;
    switch (event.type)
    {
    case SDL_QUIT:
        stopRendering = true;
        break;

	case SDL_KEYDOWN:
        key = event.key.keysym.sym;

		if (key == SDLK_ESCAPE)
			stopRendering = true;
        break;

    case SDL_MOUSEBUTTONDOWN:
        if (event.button.button == SDL_BUTTON_LEFT)
        {
            leftMouseDown = true;
            lastMouseX = event.button.x;
            lastMouseY = event.button.y;
        } else if (event.button.button == SDL_BUTTON_RIGHT)
        {
            rightMouseDown = true;
            lastMouseX = event.button.x;
            lastMouseY = event.button.y;
        } else if (event.button.button == SDL_BUTTON_MIDDLE)
        {
            middleMouseDown = true;
            lastMouseX = event.button.x;
            lastMouseY = event.button.y;
        }
        break;

    case SDL_MOUSEBUTTONUP:
        if (event.button.button == SDL_BUTTON_LEFT)
        {    
            leftMouseDown = false;
        } else if (event.button.button == SDL_BUTTON_RIGHT)
        {
            rightMouseDown = false;
        } else if (event.button.button == SDL_BUTTON_MIDDLE)
        {
            middleMouseDown = false;
        }
        break;

    case SDL_MOUSEMOTION:
        handleMouseMotion(event);
        break;

	case SDL_MOUSEWHEEL:
	{
        vec3 moveForward, moveRight, worldUp;
        computeCameraBasis(moveForward, moveRight, worldUp);

		cameraPosition += moveForward * (event.wheel.y * zoomSpeed);
	}
		break;

    default:
        break;
    }
}

void updateCamera(float deltaTimeSeconds)
{
    const Uint8* keyboardState = SDL_GetKeyboardState(nullptr);
    vec3 moveForward, moveRight, worldUp;
    computeCameraBasis(moveForward, moveRight, worldUp);

    const float moveSpeed = cameraMoveSpeed * deltaTimeSeconds;

    if (keyboardState[SDL_SCANCODE_W])
    {
        cameraPosition += moveForward * moveSpeed;
    }
    if (keyboardState[SDL_SCANCODE_S])
    {
        cameraPosition -= moveForward * moveSpeed;
    }
    if (keyboardState[SDL_SCANCODE_A])
    {
        cameraPosition -= moveRight * moveSpeed;
    }
    if (keyboardState[SDL_SCANCODE_D])
    {
        cameraPosition += moveRight * moveSpeed;
    }
    if (keyboardState[SDL_SCANCODE_Z])
    {
        cameraPosition += worldUp * moveSpeed;
    }
    if (keyboardState[SDL_SCANCODE_C])
    {
        cameraPosition -= worldUp * moveSpeed;
    }
}
