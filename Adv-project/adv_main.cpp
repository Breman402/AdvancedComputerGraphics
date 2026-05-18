#include <GL/glew.h>
#include <labhelper.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <imgui.h>
#include <imgui_impl_sdl_gl3.h>

#include <map>
#include <vector>
#include <cstdint>
#include <algorithm>
#include <cmath>
#include <limits>

#include "loadTexture.hpp"
#include "sphereMesh.hpp"
#include "gui.hpp"
#include "applicationIcon.hpp"
#include "scatter.hpp"
#include "shaderUniforms.hpp"

#include "camera.hpp"
#include "createShadow.hpp"
#include "sun.hpp"

using namespace glm;

// Settings
constexpr float SQUARE_SIZE = 64.0f; // 64.0
constexpr int gridRes = 128; // (128) number of quads per terrain tile, this determines how detailed the terrain can be within a single tile
constexpr int terrainTileSeed = 1337; // random seed for terrain generation, this can be used in the shader to generate different noise patterns for different tiles
constexpr int renderDistance = 20; // (20) how many terrain tiles to render in each direction from the camera, so renderDistance=1 means only the 8 tiles surrounding the camera and the one the camera is on will be rendered, renderDistance=2 means a 5x5 grid of tiles will be rendered etc.
constexpr int terrainCacheResolution = (renderDistance * 2 + 1) * gridRes + 1;
constexpr float terrainMaxHeight = 77.0f; // (77.0)
GUISettings guiSettings;

ShaderUniforms shaderUniforms;

// Structs

/**
 * @brief A struct representing a single terrain square, its world position and its limits.
 */
struct TerrainSquare
{
    vec3 center = glm::vec3(0.0f, 0.0f, 0.0f);; // world position of the center of the square, acts as an ID and coordinates
    float width = SQUARE_SIZE; // width of the square in world units
    float length = width; // length of the square in world units, for now we will keep it the same as width but this allows for non square rectangles in the future if needed
    float height = terrainMaxHeight; // Maximum allowed height of the terrain in this square in relation to the width.
};

// Globals
SDL_Window* g_window = nullptr;
GLuint terrainShader = 0;
GLuint terrainCacheProgram = 0;

// Terrain textures
ShaderTextureHandles shaderTextures;
TerrainTileMesh terrainTileMesh;

// Scatter
ScatterObject treeScatterObject(
    "Tree", // Name
    {0.5f, 2.0f}, // Scale
    0.0f, // Offset
    {TerrainType::gras, TerrainType::gras}, // Terrain range
    64.0f, // Grid size
    100, // Frequency
    "../Adv-project/models/Tree/Tree.obj" // .obj path
); 
std::vector<ScatterObject*> scatterObjects = { &treeScatterObject };
ScatterObjectRenderer* scatterRenderer = nullptr;


// Lighting
mat4 lightViewMatrix;
mat4 lightProjectionMatrix;
glm::vec3 lightDirWorld = glm::vec3(0.0f, 1.0f, 0.0f);

// Sun related Globals
GLuint sunProgram = 0;
SphereMesh g_sunSphere;

// Atmosphere related Globals
GLuint atmosphereProgram = 0;
SphereMesh g_atmosphereSphere;
SunLightingState sunLighting;
constexpr float kBaseSkySunIntensity = 0.35f; // The minimum solar energy sent into the atmosphere
constexpr float kReferenceLightIntensity = 5.3f; // Used to normalize the slider
constexpr float kBaseVisibleSunIntensity = 0.25f; //the minimum brightness of the visible sun disc before we apply intensity scaling and the horizon visibility fade.

// Shadow related Globals
GLuint shadowProgram = 0;
ShadowMap shadowMap;


// A sample square with information later passed to the GPU
TerrainSquare sampleSquare;
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

    // OpenGL settings
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f); // black background

    lightViewMatrix = mat4(1.0f);
    lightProjectionMatrix = mat4(1.0f);
    lightDirWorld = normalize(glm::vec3(-0.4f, -1.0f, -0.3f));
    syncSunCycleFromLightDirection(lightDirWorld, guiSettings.sunTimeOfDayAngle);

    // This is just for setting an icon for the application window.
    setApplicationIcon(g_window, "../Adv-project/icons/earthIcon.bmp");

    terrainShader = labhelper::loadShaderProgram(
        "../Adv-project/terrain.vert",
        "../Adv-project/terrain.frag"
    );

    terrainCacheProgram = labhelper::loadShaderProgram(
        "../Adv-project/terrainCache.vert",
        "../Adv-project/terrainCache.frag"
    );

	shadowProgram = labhelper::loadShaderProgram(
		"../Adv-project/shadowMap.vert",
		"../Adv-project/shadowMap.frag"
	);

    sunProgram = labhelper::loadShaderProgram(
        "../Adv-project/sun.vert",
        "../Adv-project/sun.frag"
    );

    atmosphereProgram = labhelper::loadShaderProgram(
        "../Adv-project/atmosphere.vert",
        "../Adv-project/atmosphere.frag"
    );

    scatterRenderer = new ScatterObjectRenderer(
        scatterObjects,
        "../Adv-project/scatter.vert",
        "../Adv-project/scatter.frag"
    );

	// Create shadow map
	shadowMap = createShadowMap(guiSettings.shadowMapSize);
    guiSettings.shadowMap = &shadowMap; // So that the shadow map can be changed later from the GUI
    shaderUniforms.track(guiSettings, lightViewMatrix, lightProjectionMatrix, lightDirWorld, cameraPosition, sunLighting, shadowMap);

	// Load the inital terrain textures:
    shaderTextures.water = loadTexture("../Adv-project/textures/water.png");
    shaderTextures.sand = loadTexture("../Adv-project/textures/sand.png");
    shaderTextures.grass[0] = loadTexture("../Adv-project/textures/grass.png");
    shaderTextures.grass[1] = loadTexture("../Adv-project/textures/grass2.png");
    shaderTextures.grass[2] = loadTexture("../Adv-project/textures/grass3.png");
    shaderTextures.grass[3] = loadTexture("../Adv-project/textures/grass4.png");
    shaderTextures.rock[0] = loadTexture("../Adv-project/textures/rock.png");
    shaderTextures.rock[1] = loadTexture("../Adv-project/textures/rock2.png");
    shaderTextures.rock[2] = loadTexture("../Adv-project/textures/rock3.png");
    shaderTextures.rock[3] = loadTexture("../Adv-project/textures/rock4.png");
    shaderTextures.snow[0] = loadTexture("../Adv-project/textures/snow.png");
    shaderTextures.snow[1] = loadTexture("../Adv-project/textures/snow2.png");
    shaderTextures.snow[2] = loadTexture("../Adv-project/textures/snow3.png");
    shaderTextures.snow[3] = loadTexture("../Adv-project/textures/snow4.png");

    // Use the night panorama as a star field on top of the procedural atmosphere.
    shaderTextures.starField = loadTexture("../Adv-project/textures/skyNightTime.png");

    // Create the reusable tile geometry once, then let the shaders displace it.
    terrainTileMesh.create(gridRes);
    terrainTileMesh.createCache(terrainCacheResolution);

    // Create the Sun
    g_sunSphere = createSphereMesh(1.0f, 50, 50); // low res sphere for sun

    // Spawn the camera above the middle tile so the terrain is clearly in view on frame one.
    cameraPosition = glm::vec3(256.0f, 160.0f, 420.0f);

    // Create the atmosphere shell.
    g_atmosphereSphere = createSphereMesh(2000.0f, 64, 64);

}

void display()
{
    int w = 0, h = 0;
    SDL_GetWindowSize(g_window, &w, &h);

    const float aspect = float(w) / float(h ? h : 1);
    const mat4 projection = perspective(radians(60.0f), aspect, 0.1f, 3000.0f);

    constexpr float terrainSquareSize = SQUARE_SIZE;
    const int cameraGridX = static_cast<int>(std::floor(cameraPosition.x / terrainSquareSize));
    const int cameraGridZ = static_cast<int>(std::floor(cameraPosition.z / terrainSquareSize));

    const ivec2 terrainCacheOriginGrid(cameraGridX - renderDistance, cameraGridZ - renderDistance);
    const vec3 terrainCacheOriginWorld = terrainTileMesh.cacheOriginWorld(terrainCacheOriginGrid, sampleSquare.width);
    const float terrainVertexSpacing = terrainTileMesh.cacheVertexSpacing(sampleSquare.width);

    if (terrainTileMesh.cacheNeedsUpdate(terrainCacheOriginGrid, guiSettings.heightMapScale, sampleSquare.width, sampleSquare.height, terrainTileSeed))
    {
        terrainTileMesh.updateCache(terrainCacheProgram, terrainCacheOriginGrid, guiSettings.heightMapScale, sampleSquare.width, sampleSquare.height, terrainTileSeed);
    }
    shaderUniforms.setTerrainCache(
        terrainCacheOriginWorld,
        terrainVertexSpacing,
        terrainTileMesh.cacheResolution(),
        sampleSquare.height,
        terrainTileMesh.heightTexture(),
        terrainTileMesh.normalTexture()
    );

    updateShadowMatrices(cameraGridX, cameraGridZ, terrainSquareSize, renderDistance, sampleSquare.height, lightDirWorld, lightViewMatrix, lightProjectionMatrix);

    // ---- Camera ----
    const float totalYaw = cameraYaw + viewYawOffset;
    const float totalPitch = glm::clamp(cameraPitch + viewPitchOffset, radians(-89.0f), radians(89.0f));
    const glm::vec3 F_rot = normalize(glm::vec3(
        sin(totalYaw) * cos(totalPitch),
        sin(totalPitch),
        -cos(totalYaw) * cos(totalPitch)
    ));
    const glm::vec3 R_rot = normalize(cross(F_rot, glm::vec3(0.0f, 1.0f, 0.0f)));
    const glm::vec3 U_rot = normalize(cross(R_rot, F_rot));

    mat4 view = lookAt(glm::vec3(cameraPosition), glm::vec3(cameraPosition + F_rot), glm::vec3(U_rot));

    // ------------------------------------------------------------------------
    // Shadow pass
    // ------------------------------------------------------------------------
    glViewport(0, 0, shadowMap.width, shadowMap.height);

    glBindFramebuffer(GL_FRAMEBUFFER, shadowMap.fbo);
    glClear(GL_DEPTH_BUFFER_BIT);

    glUseProgram(shadowProgram);

    shaderUniforms.uploadShadowTerrain(shadowProgram);
    shaderUniforms.bindTerrainHeightTexture();
    labhelper::setUniformSlow(shadowProgram, "scatterShadowMode", false);
    
    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(2.0f, 4.0f);

    terrainTileMesh.bind();

    for (int dz = -renderDistance; dz <= renderDistance; ++dz)
    {
        for (int dx = -renderDistance; dx <= renderDistance; ++dx)
        {
            const int gridX = cameraGridX + dx;
            const int gridZ = cameraGridZ + dz;

            const vec3 tileCenter(
                (gridX + 0.5f) * terrainSquareSize,
                0.0f,
                (gridZ + 0.5f) * terrainSquareSize
            );

            mat4 tileModel = translate(mat4(1.0f), tileCenter) *
                             scale(mat4(1.0f), vec3(float(terrainSquareSize), 1.0f, float(terrainSquareSize)));

            labhelper::setUniformSlow(shadowProgram, "modelMatrix", tileModel);

            terrainTileMesh.draw();
        }
    }

    scatterRenderer->renderShadow(
        cameraGridX,
        cameraGridZ,
        renderDistance,
        float(terrainSquareSize),
        shaderUniforms,
        shadowProgram
    );

    glDisable(GL_POLYGON_OFFSET_FILL);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // Evaluate the sun lighting based on current light direction
    sunLighting = evaluateSunLighting(lightDirWorld, guiSettings.lightIntensity);
    const float intensityScale = guiSettings.lightIntensity / kReferenceLightIntensity;
    const float skySunIntensity = kBaseSkySunIntensity + intensityScale;
    const float visibleSunIntensity = (kBaseVisibleSunIntensity + intensityScale) * sunLighting.sunVisibility;

    // ------------------------------------------------------------------------
    // Main pass
    // ------------------------------------------------------------------------
    glViewport(0, 0, w, h);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glUseProgram(terrainShader);

    shaderUniforms.bindTerrainMaterialTextures(shaderTextures);

    if (!guiSettings.uploadGuard)
    {
        shaderUniforms.uploadTerrainStatic(terrainShader);
        guiSettings.uploadGuard = true;
    }

    shaderUniforms.uploadTerrainFrame(terrainShader);
    shaderUniforms.bindShadowTexture();
    shaderUniforms.bindTerrainCacheTextures();

    if (guiSettings.wireframeMode)
    {
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    }

    terrainTileMesh.bind();

    for (int dz = -renderDistance; dz <= renderDistance; ++dz)
    {
        for (int dx = -renderDistance; dx <= renderDistance; ++dx)
        {
            const int gridX = cameraGridX + dx;
            const int gridZ = cameraGridZ + dz;

            const vec3 tileCenter(
                (gridX + 0.5f) * terrainSquareSize,
                0.0f,
                (gridZ + 0.5f) * terrainSquareSize
            );

            mat4 tileModel = translate(mat4(1.0f), tileCenter) *
                             scale(mat4(1.0f), vec3(float(terrainSquareSize), 1.0f, float(terrainSquareSize)));

            mat4 tileMvp = projection * view * tileModel;

            labhelper::setUniformSlow(terrainShader, "modelMatrix", tileModel);
            labhelper::setUniformSlow(terrainShader, "modelViewProjectionMatrix", tileMvp);

            terrainTileMesh.draw();
        }
    }

    if (guiSettings.wireframeMode)
    {
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }
    // ------------------------------------------------------------------------
    // Render all scatter objects
    // ------------------------------------------------------------------------
    scatterRenderer->render(
        cameraGridX,
        cameraGridZ,
        renderDistance,
        float(terrainSquareSize),
        projection,
        view,
        shaderUniforms
    );

    // ------------------------------------------------------------------------
    // Draw the atmosphere
    // ------------------------------------------------------------------------
    glUseProgram(atmosphereProgram);

    vec3 atmospherePosWorld = cameraPosition;

    mat4 atmosphereModel(1.0f);
    atmosphereModel = translate(atmosphereModel, atmospherePosWorld);

    mat4 atmosphereMvp = projection * view * atmosphereModel;
    labhelper::setUniformSlow(atmosphereProgram, "mvpMatrix", atmosphereMvp);
    shaderUniforms.uploadAtmosphere(atmosphereProgram, skySunIntensity);
    shaderUniforms.bindStarTexture(shaderTextures.starField);

    glBindVertexArray(g_atmosphereSphere.vao);
    glDepthMask(GL_FALSE); // No depth mask so that the terrain can be seen through the atmosphere
    glDisable(GL_CULL_FACE);
    glDrawElements(GL_TRIANGLES, g_atmosphereSphere.indexCount, GL_UNSIGNED_INT, 0);
    glEnable(GL_CULL_FACE);
    glDepthMask(GL_TRUE);

    // ------------------------------------------------------------------------
    // Draw the Sun
    // ------------------------------------------------------------------------
    glUseProgram(sunProgram);

    vec3 sunPosWorld = cameraPosition - normalize(lightDirWorld) * guiSettings.sunDistance;

    mat4 sunModel(1.0f);
    sunModel = translate(sunModel, sunPosWorld);
    sunModel = scale(sunModel, vec3(guiSettings.sunRadiusWorld));

    mat4 sunMvp = projection * view * sunModel;
    shaderUniforms.uploadSun(sunProgram, sunMvp, sunModel, visibleSunIntensity);

    glBindVertexArray(g_sunSphere.vao);
    glDepthMask(GL_FALSE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    glDrawElements(GL_TRIANGLES, g_sunSphere.indexCount, GL_UNSIGNED_INT, 0);
    glDisable(GL_BLEND);
    glDepthMask(GL_TRUE);

    glBindVertexArray(0);
}

int main(int argc, char* argv[])
{
    g_window = labhelper::init_window_SDL("Dynamic Terrain Viewer");

    initialize();

    bool stopRendering = false;
    Uint32 previousTicks = SDL_GetTicks();
    while (!stopRendering)
    {
        const Uint32 currentTicks = SDL_GetTicks();
        const float deltaTimeSeconds = float(currentTicks - previousTicks) / 1000.0f;
        previousTicks = currentTicks;

        SDL_Event event;
        // Allow ImGui to capture events.
	    ImGuiIO& io = ImGui::GetIO();    

        while (SDL_PollEvent(&event))
        {
            ImGui_ImplSdlGL3_ProcessEvent(&event);
            handleSDLEvents(event, stopRendering);
        }

        updateCamera(deltaTimeSeconds, guiSettings.cameraMoveSpeed);
        updateSunFromKeyboard(deltaTimeSeconds, lightDirWorld, guiSettings.sunTimeOfDayAngle);

        // Inform imgui of new frame
        ImGui_ImplSdlGL3_NewFrame(g_window);

        display();

        // Render overlay GUI.
        gui(guiSettings);

        // Render the GUI.
        ImGui::Render();

        SDL_GL_SwapWindow(g_window);
    }

    //cleanup();
    ImGui_ImplSdlGL3_Shutdown();
    labhelper::shutDown(g_window);
    return 0;
}
