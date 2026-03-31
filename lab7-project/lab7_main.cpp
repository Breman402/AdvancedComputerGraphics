#include <GL/glew.h>
#include <labhelper.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <imgui.h>
#include <imgui_impl_sdl_gl3.h>

#include "sphereMesh.hpp"
#include "loadTexture.hpp"
#include "createShadow.hpp"
#include "gui.hpp"
#include "applicationIcon.hpp"
#include "applicationIcon.hpp"

using namespace glm;

// Globals
SDL_Window* g_window = nullptr;
GLuint earthShader = 0;
GLuint shadowProgram = 0;
SphereMesh g_sphere;

// Earth texture options
EarthTextureToUse earthTextureOption;
GLuint* currentEarthTexture;
GLuint earthTexture;
GLuint earthTexture8kPNG;
GLuint earthTextureBlueMarbleJPEG;
GLuint earthHeightTexture;
Material earthMaterial;

float heightMapScale = 0.05f;
//float heightMapScale = 0.5f;

float lightAngle = 0.0f;
float lightIntensity = 5.0f;
float lightRotationSpeed = 0.1f;
float lightIntensitySpeed = 0.1f;

mat4 lightViewMatrix;
mat4 lightProjectionMatrix;

GLuint shadowFbo = 0;
GLuint shadowDepthTex = 0;
ShadowMap shadowMap;
int shadowMapSize = 4048;


// Mouse rotation state
bool leftMouseDown = false;
bool rightMouseDown = false;
bool middleMouseDown = false;
int  lastMouseX = 0;
int  lastMouseY = 0;

// Globe rotation
float yawAngle   = 0.0f; // rotate around Y
float pitchAngle = 0.0f; // rotate around X

// Camera rotation & controlls
float cameraYaw = 0.0f;
float cameraPitch = 0.0f;
float viewYawOffset = 0.0f;
float viewPitchOffset = 0.0f;
float cameraDistance = 3.0f; // 3 × radius keeps the whole Earth visible
float earthRadius   = 1.0f;
float cameraOrbitSpeed    = 0.05f;

// Sun related Globlas
GLuint sunProgram = 0;
SphereMesh g_sunSphere;
float sunDistance    = 10.0f;         // how far from Earth center
float sunRadiusWorld = 0.2f;          // size of the visible sun in world units

// Wireframe mode
bool wireframeMode = false;
bool showTexture = true;

//----------------------------------------------------------------------------
// Init / Shutdown
//----------------------------------------------------------------------------
void initialize()
{
    if (glewInit() != GLEW_OK)
    {
        fprintf(stderr, "Failed to initialize GLEW\n");
        std::exit(EXIT_FAILURE);
    }

    glEnable(GL_DEPTH_TEST);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f); // black background

    // This is just for setting an icon for the application window.
    setApplicationIcon(g_window, "../lab7-project/earthIcon.bmp");

    earthShader = labhelper::loadShaderProgram(
        "../lab7-project/earth.vert",
        "../lab7-project/earth.frag"
    );

	shadowProgram = labhelper::loadShaderProgram(
		"../lab7-project/shadowMap.vert",
		"../lab7-project/shadowMap.frag"
	);

    sunProgram = labhelper::loadShaderProgram(
        "../lab7-project/sun.vert",
        "../lab7-project/sun.frag"
    );

	g_sphere = createSphereMesh(earthRadius, 800, 2200); // More slices, more triangles pick a more preciise part of the texture

	// Create shadow map
	shadowMap = createShadowMap(shadowMapSize);

	// Load the Earth Textures, user later decides which one to use:
	earthTexture = loadTexture("../lab7-project/EarthTexture.jpg");
    earthTexture8kPNG = loadTexture("../lab7-project/8192px-Blue_Marble_2002.png");
    earthTextureBlueMarbleJPEG = loadTexture("../lab7-project/theworld.jpg");

	// Load the Earth Height Map:
	earthHeightTexture = loadTexture("../lab7-project/EarthHeightMap.jpg");

    // Create the Sun
    g_sunSphere = createSphereMesh(1.0f, 50, 50); // low res sphere for sun

    // Default earth material properties:
    earthMaterial.color = vec3(1.0f, 1.0f, 1.0f);
    earthMaterial.metalness = 0.0f;
    earthMaterial.fresnel = 0.062f;
    earthMaterial.shininess = 57.862f;
    earthMaterial.emission = vec3(0.0f, 0.0f, 0.01f);

}

void cleanup()
{
    if (g_sphere.ebo) glDeleteBuffers(1, &g_sphere.ebo);
    if (g_sphere.vbo) glDeleteBuffers(1, &g_sphere.vbo);
    if (g_sphere.vao) glDeleteVertexArrays(1, &g_sphere.vao);
}

//----------------------------------------------------------------------------
// Input handling
//----------------------------------------------------------------------------
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

    const float zoomSpeed = 0.05f * cameraDistance;
    switch (event.type)
    {
    case SDL_QUIT:
        stopRendering = true;
        break;

	case SDL_KEYDOWN:
        key = event.key.keysym.sym;

		if (key == SDLK_ESCAPE)
			stopRendering = true;

		// ---- Light rotation left/right ----
		if (key == SDLK_LEFT)
			lightAngle -= lightRotationSpeed;        // rotate sun left

		if (key == SDLK_RIGHT)
			lightAngle += lightRotationSpeed;        // rotate sun right

		// ---- Light intensity up/down ----
		if (key == SDLK_UP)
			lightIntensity = std::min(lightIntensity + lightIntensitySpeed, 5.0f);  // cap at 5×

		if (key == SDLK_DOWN)
			lightIntensity = std::max(lightIntensity - lightIntensitySpeed, 0.0f);  // no negative light
        
        // ---- Camera WASD controls ----
        if (key == SDLK_w)
        {
            cameraDistance -= (zoomSpeed * 0.1f);
        }
        if (key == SDLK_s)
        {
            cameraDistance += (zoomSpeed * 0.1f);
        }
        if (key == SDLK_a)
        {
            cameraYaw -= (cameraOrbitSpeed * 0.1f);
        }
        if (key == SDLK_d)
        {
            cameraYaw += (cameraOrbitSpeed * 0.1f);
        }
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
		cameraDistance -= event.wheel.y * zoomSpeed;
		if (cameraDistance <= earthRadius + 0.3f) cameraDistance = earthRadius + 0.3f;
		break;

    default:
        break;
    }
}

//----------------------------------------------------------------------------
// Render
//----------------------------------------------------------------------------
void display()
{

    // Set what texture to use for the Earth
    if(earthTextureOption.useBlueMarbel8kPNG)
    {
        currentEarthTexture = &earthTexture8kPNG;
    }
     else if(earthTextureOption.useMapMode)
    {
        currentEarthTexture = &earthTextureBlueMarbleJPEG;
    }
    else if(earthTextureOption.useGrayScaleHeightMap)
    {
        currentEarthTexture = &earthHeightTexture;
    }
    else
    {
        currentEarthTexture = &earthTexture;
    }
    int w = 0, h = 0;
    SDL_GetWindowSize(g_window, &w, &h);

    const float aspect = float(w) / float(h ? h : 1);
    const mat4 projection = perspective(radians(60.0f), aspect, 0.001f, 100.0f);

    // ---- Camera ----

    float x = cameraDistance * cos(cameraPitch) * sin(cameraYaw);
    float y = cameraDistance * sin(cameraPitch);
    float z = cameraDistance * cos(cameraPitch) * cos(cameraYaw);
    vec3 cameraPos = vec3(x, y, z);
    // Base forward direction: towards Earth center
    vec3 F = normalize(-cameraPos);       // from camera to origin
    vec3 worldUp = vec3(0.0f, 1.0f, 0.0f);
    vec3 R = normalize(cross(F, worldUp));
    vec3 U = normalize(cross(R, F));

    // Apply view yaw/pitch offsets around local axes
    mat4 yawRot   = glm::rotate(mat4(1.0f), viewYawOffset,   U); // look left/right
    mat4 pitchRot = glm::rotate(mat4(1.0f), viewPitchOffset, R); // look up/down

    vec3 F_rot = glm::vec3(yawRot * pitchRot * vec4(F, 0.0f));
    F_rot = normalize(F_rot);

    // Recompute up after rotation
    vec3 U_rot = normalize(cross(R, F_rot));

    mat4 view = lookAt(
        cameraPos,
        cameraPos + F_rot,
        U_rot
    );


    // ---- Model (rotation only) ----
    mat4 model(1.0f);
    model = rotate(model, pitchAngle, vec3(1.0f, 0.0f, 0.0f));
    model = rotate(model, yawAngle,   vec3(0.0f, 1.0f, 0.0f));

    mat4 mvp = projection * view * model;

    // ---- Light direction & light matrices ----
    vec3 lightDirWorld = normalize(vec3(cos(lightAngle), 0.3f, sin(lightAngle)));
    vec3 lightPosWorld = -lightDirWorld * 10.0f;  // somewhere along -dir

    mat4 lightView = lookAt(
        lightPosWorld,
        vec3(0.0f, 0.0f, 0.0f),
        vec3(0.0f, 1.0f, 0.0f)
    );

    float r = earthRadius * 1.5f;
    mat4 lightProj = ortho(-r, r, -r, r, 0.1f, 20.0f);
    mat4 lightViewProj = lightProj * lightView;

    // =====================================================================
    // 1) SHADOW PASS: render depth from light's POV into g_shadowMap
    // =====================================================================
    glViewport(0, 0, shadowMap.width, shadowMap.height);
    glBindFramebuffer(GL_FRAMEBUFFER, shadowMap.fbo);
    glClearDepth(1.0);
    glClear(GL_DEPTH_BUFFER_BIT);

    glUseProgram(shadowProgram);

    // Pass lightViewProj & model to shadowMap.vert
    labhelper::setUniformSlow(shadowProgram, "lightViewProj", lightViewProj);
    labhelper::setUniformSlow(shadowProgram, "modelMatrix", model);

    // Heightmap (unit 1) for displacement in shadow pass
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, earthHeightTexture);
    labhelper::setUniformSlow(shadowProgram, "earthHeightTex", 1);
    labhelper::setUniformSlow(shadowProgram, "baseRadius", earthRadius);
    labhelper::setUniformSlow(shadowProgram, "heightScale", heightMapScale);

    glBindVertexArray(g_sphere.vao);
    glDrawElements(GL_TRIANGLES, g_sphere.indexCount, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // =====================================================================
    // 2) MAIN PASS: normal render, sampling the shadow map
    // =====================================================================
    glViewport(0, 0, w, h);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glUseProgram(earthShader);

    // Color map (unit 0)
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, *currentEarthTexture);
    labhelper::setUniformSlow(earthShader, "earthTex", 0);

    // Height map (unit 1)
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, earthHeightTexture);
    labhelper::setUniformSlow(earthShader, "earthHeightTex", 1);

    // Shadow map (unit 2)
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, shadowMap.depthTex);
    labhelper::setUniformSlow(earthShader, "shadowMap", 2);

    // Matrices
    labhelper::setUniformSlow(earthShader, "modelViewProjectionMatrix", mvp);
    labhelper::setUniformSlow(earthShader, "modelMatrix", model);
    labhelper::setUniformSlow(earthShader, "lightViewProj", lightViewProj);

    // Heightmap params
    labhelper::setUniformSlow(earthShader, "baseRadius", earthRadius);
    labhelper::setUniformSlow(earthShader, "heightScale", heightMapScale);

    // Lighting
    labhelper::setUniformSlow(earthShader, "lightDirWorld", lightDirWorld);
    labhelper::setUniformSlow(earthShader, "cameraPosWorld", cameraPos);
    labhelper::setUniformSlow(earthShader,
        "point_light_intensity_multiplier", lightIntensity);

    // Material
    // Check if there has been a change in the material properties,
    // if so update the earth's material uniform
    static Material oldEarthMaterial;
    if (memcmp(&oldEarthMaterial, &earthMaterial, sizeof(Material)) != 0)
    {
        labhelper::setUniformSlow(earthShader, "earthMaterial.color", earthMaterial.color);
        labhelper::setUniformSlow(earthShader, "earthMaterial.metalness", earthMaterial.metalness);
        labhelper::setUniformSlow(earthShader, "earthMaterial.fresnel", earthMaterial.fresnel);
        labhelper::setUniformSlow(earthShader, "earthMaterial.shininess", earthMaterial.shininess);
        labhelper::setUniformSlow(earthShader, "earthMaterial.emission", earthMaterial.emission);
        oldEarthMaterial = earthMaterial;
    }

    // Wireframe mode, this is passed cause if wireframe mode is on, the texture should not be shown
    labhelper::setUniformSlow(earthShader, "wireframeMode", wireframeMode);
    labhelper::setUniformSlow(earthShader, "showTexture", showTexture);

    // Apply wireframe mode if enabled
    if (wireframeMode)
    {
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    }

    glBindVertexArray(g_sphere.vao);
    glDrawElements(GL_TRIANGLES, g_sphere.indexCount, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0); // unbind

    // Restore normal polygon mode
    if (wireframeMode)
    {
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }

    // Draw the Sun
    glUseProgram(sunProgram);

    // Position of the sun = light source position
    vec3 sunPosWorld = lightPosWorld;

    mat4 sunModel(1.0f);
    sunModel = translate(sunModel, sunPosWorld);
    sunModel = scale(sunModel, vec3(sunRadiusWorld));

    mat4 sunMvp = projection * view * sunModel;
    labhelper::setUniformSlow(sunProgram, "mvpMatrix", sunMvp);
    labhelper::setUniformSlow(sunProgram, "sunColor",
                            vec3(1.0f, 1.0f, 0.8f) * lightIntensity);

    glBindVertexArray(g_sunSphere.vao);
    glDrawElements(GL_TRIANGLES, g_sunSphere.indexCount, GL_UNSIGNED_INT, 0);

    glBindVertexArray(0); // unbind
}


//----------------------------------------------------------------------------
// Main
//----------------------------------------------------------------------------
int main(int argc, char* argv[])
{
    g_window = labhelper::init_window_SDL("Earth Viewer");

    initialize();

    bool stopRendering = false;
    while (!stopRendering)
    {
        SDL_Event event;
        // Allow ImGui to capture events.
	    ImGuiIO& io = ImGui::GetIO();    

        while (SDL_PollEvent(&event))
        {
            ImGui_ImplSdlGL3_ProcessEvent(&event);
            handleSDLEvents(event, stopRendering);
        }

        // Inform imgui of new frame
        ImGui_ImplSdlGL3_NewFrame(g_window);

        display();

        // Render overlay GUI.
        gui(g_sphere, heightMapScale, shadowMapSize, shadowMap, earthMaterial, wireframeMode, showTexture, earthTextureOption);

        // Render the GUI.
        ImGui::Render();

        SDL_GL_SwapWindow(g_window);
    }

    cleanup();
    ImGui_ImplSdlGL3_Shutdown();
    labhelper::shutDown(g_window);
    return 0;
}
