#include <GL/glew.h>
#include <labhelper.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <imgui.h>
#include <imgui_impl_sdl_gl3.h>

#include <map>
#include <vector>
#include <cstdint>
#include <cmath>

#include "loadTexture.hpp"
#include "sphereMesh.hpp"
#include "gui.hpp"
#include "applicationIcon.hpp"
#include "applicationIcon.hpp"
#include "camera.hpp"
#include "createShadow.hpp"

using namespace glm;

// Settings
#define SQUARE_SIZE 64 // 64
constexpr int gridRes = 128; // (128) number of quads per terrain tile, this determines how detailed the terrain can be within a single tile
constexpr int terrainTileSeed = 1337; // random seed for terrain generation, this can be used in the shader to generate different noise patterns for different tiles
constexpr int renderDistance = 20; // (20) how many terrain tiles to render in each direction from the camera, so renderDistance=1 means only the 8 tiles surrounding the camera and the one the camera is on will be rendered, renderDistance=2 means a 5x5 grid of tiles will be rendered etc.
constexpr float terrainToGridWidthRatio = 0.4f; // (1.2)
GUISettings guiSettings;

// Structs

/**
 * @brief A struct representing a single terrain square, its world position and its limits.
 */
struct TerrainSquare
{
    vec3 center = glm::vec3(0.0f, 0.0f, 0.0f);; // world position of the center of the square, acts as an ID and coordinates
    float width = SQUARE_SIZE; // width of the square in world units
    float length = width; // length of the square in world units, for now we will keep it the same as width but this allows for non square rectangles in the future if needed
    float height = width*terrainToGridWidthRatio; // Maximum allowed height of the terrain in this square in relation to the width.
};

/**
 * @brief An enum representing the terrain texture to be used.
 */
enum class TerrainTextureType : GLuint
{
    Water,
    Sand,
    Grass,
    Rock,
    Snow
};

/**
 * @brief A struct representing a single terrain vertex, its postion, its normal and a texture coordinate.
 */
struct TerrainVertex
{
    glm::vec3 position;
    glm::vec3 normal;
    vec2 texcoord;
    TerrainTextureType textureType;
};

// Globals
SDL_Window* g_window = nullptr;
GLuint terrainShader = 0;

// Terrain
GLuint terrainWaterTexture;GLuint terrainSandTexture;GLuint terrainGrassTexture;GLuint terrainRockTexture;GLuint terrainSnowTexture;GLuint terrainHeightTexture;
GLuint terrainTileVao = 0;
GLuint terrainTileVbo = 0;
GLuint terrainTileEbo = 0;
GLsizei terrainTileIndexCount = 0;

// Lighting
mat4 lightViewMatrix;
mat4 lightProjectionMatrix;
glm::vec3 lightDirWorld = glm::vec3(0.0f, 1.0f, 0.0f);
float sunAzimuth = 0.0f;
float sunElevation = glm::radians(45.0f);

// Sun related Globals
GLuint sunProgram = 0;
SphereMesh g_sunSphere;
float sunDistance    = 1200.0f;       // how far from the terrain origin the sun is drawn
float sunRadiusWorld = 70.0f;         // size of the visible sun in world units

// Shadow related Globals
GLuint shadowProgram = 0;
ShadowMap shadowMap;

bool showTexture = true;


// A sample square with information later passed to the GPU
TerrainSquare sampleSquare;

namespace
{
constexpr float kSunRotationSpeed = glm::radians(45.0f);
constexpr float kMinSunElevation = glm::radians(2.0f);
constexpr float kMaxSunElevation = glm::radians(89.0f);

void syncLightDirectionFromSunAngles()
{
    const vec3 sunDirection = normalize(vec3(
        cos(sunElevation) * sin(sunAzimuth),
        sin(sunElevation),
        -cos(sunElevation) * cos(sunAzimuth)
    ));

    lightDirWorld = -sunDirection;
}

void syncSunAnglesFromLightDirection()
{
    const vec3 sunDirection = normalize(-lightDirWorld);
    sunElevation = glm::clamp(std::asin(sunDirection.y), kMinSunElevation, kMaxSunElevation);
    sunAzimuth = std::atan2(sunDirection.x, -sunDirection.z);
    syncLightDirectionFromSunAngles();
}

void updateSunFromKeyboard(float deltaTimeSeconds)
{
    ImGuiIO& io = ImGui::GetIO();
    if (io.WantCaptureKeyboard)
    {
        return;
    }

    const Uint8* keyboardState = SDL_GetKeyboardState(nullptr);
    const float angleStep = kSunRotationSpeed * deltaTimeSeconds;

    if (keyboardState[SDL_SCANCODE_LEFT])
    {
        sunAzimuth -= angleStep;
    }
    if (keyboardState[SDL_SCANCODE_RIGHT])
    {
        sunAzimuth += angleStep;
    }
    if (keyboardState[SDL_SCANCODE_UP])
    {
        sunElevation = glm::clamp(sunElevation + angleStep, kMinSunElevation, kMaxSunElevation);
    }
    if (keyboardState[SDL_SCANCODE_DOWN])
    {
        sunElevation = glm::clamp(sunElevation - angleStep, kMinSunElevation, kMaxSunElevation);
    }

    syncLightDirectionFromSunAngles();
}
}

//----------------------------------------------------------------------------
// Create a terrain tile mesh
//----------------------------------------------------------------------------
void createTerrainTileMesh()
{
    std::vector<TerrainVertex> vertices;
    std::vector<unsigned int> indices;

    vertices.reserve((gridRes + 1) * (gridRes + 1));
    indices.reserve(gridRes * gridRes * 6);

    // ------------------------------------------------------------------------
    // Generate vertices
    // Flat tile in local space:
    // x,z in [-0.5, 0.5]
    // y = 0
    // ------------------------------------------------------------------------
    for (int z = 0; z <= gridRes; ++z)
    {
        for (int x = 0; x <= gridRes; ++x)
        {
            const float u = float(x) / float(gridRes);
            const float v = float(z) / float(gridRes);

            TerrainVertex vertex{};
            vertex.position = glm::vec3(u - 0.5f, 0.0f, v - 0.5f);
            vertex.normal   = glm::vec3(0.0f, 1.0f, 0.0f);
            vertex.texcoord = glm::vec2(u, v);
            vertex.textureType = TerrainTextureType::Grass; // placeholder

            vertices.push_back(vertex);
        }
    }

    // ------------------------------------------------------------------------
    // Generate indices
    // Each quad becomes 2 triangles
    // ------------------------------------------------------------------------
    for (int z = 0; z < gridRes; ++z)
    {
        for (int x = 0; x < gridRes; ++x)
        {
            const unsigned int topLeft     = z * (gridRes + 1) + x;
            const unsigned int topRight    = topLeft + 1;
            const unsigned int bottomLeft  = (z + 1) * (gridRes + 1) + x;
            const unsigned int bottomRight = bottomLeft + 1;

            indices.push_back(topLeft);
            indices.push_back(bottomLeft);
            indices.push_back(topRight);

            indices.push_back(topRight);
            indices.push_back(bottomLeft);
            indices.push_back(bottomRight);
        }
    }

    terrainTileIndexCount = static_cast<GLsizei>(indices.size());

    // ------------------------------------------------------------------------
    // Create VAO / VBO / EBO
    // ------------------------------------------------------------------------
    glGenVertexArrays(1, &terrainTileVao);
    glBindVertexArray(terrainTileVao);

    glGenBuffers(1, &terrainTileVbo);
    glBindBuffer(GL_ARRAY_BUFFER, terrainTileVbo);
    glBufferData(
        GL_ARRAY_BUFFER,
        static_cast<GLsizeiptr>(vertices.size() * sizeof(TerrainVertex)),
        vertices.data(),
        GL_STATIC_DRAW
    );

    glGenBuffers(1, &terrainTileEbo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, terrainTileEbo);
    glBufferData(
        GL_ELEMENT_ARRAY_BUFFER,
        static_cast<GLsizeiptr>(indices.size() * sizeof(unsigned int)),
        indices.data(),
        GL_STATIC_DRAW
    );

    // ------------------------------------------------------------------------
    // Vertex attributes
    // layout(location = 0) in vec3 position;
    // layout(location = 1) in vec3 normal;
    // layout(location = 2) in vec2 texcoord;
    // ------------------------------------------------------------------------
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(
        0,
        3,
        GL_FLOAT,
        GL_FALSE,
        sizeof(TerrainVertex),
        (void*)offsetof(TerrainVertex, position)
    );

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(
        1,
        3,
        GL_FLOAT,
        GL_FALSE,
        sizeof(TerrainVertex),
        (void*)offsetof(TerrainVertex, normal)
    );

    glEnableVertexAttribArray(2);
    glVertexAttribPointer(
        2,
        2,
        GL_FLOAT,
        GL_FALSE,
        sizeof(TerrainVertex),
        (void*)offsetof(TerrainVertex, texcoord)
    );

    glBindVertexArray(0);
}


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

    // Start with a valid default until the shadow pass is implemented.
    lightViewMatrix = mat4(1.0f);
    lightProjectionMatrix = mat4(1.0f);
    lightDirWorld = normalize(glm::vec3(-0.4f, -1.0f, -0.3f));
    syncSunAnglesFromLightDirection();

    // This is just for setting an icon for the application window.
    setApplicationIcon(g_window, "../Adv-project/icons/earthIcon.bmp");

    terrainShader = labhelper::loadShaderProgram(
        "../Adv-project/terrain.vert",
        "../Adv-project/terrain.frag"
    );

	shadowProgram = labhelper::loadShaderProgram(
		"../Adv-project/shadowMap.vert",
		"../Adv-project/shadowMap.frag"
	);

    sunProgram = labhelper::loadShaderProgram(
        "../Adv-project/sun.vert",
        "../Adv-project/sun.frag"
    );

	// Create shadow map
	shadowMap = createShadowMap(guiSettings.shadowMapSize);
    guiSettings.shadowMap = &shadowMap; // So that the shadow map can be changed later from the GUI

	// Load the inital terrain textures:
    terrainWaterTexture = loadTexture("../Adv-project/textures/water.png");
    terrainSandTexture = loadTexture("../Adv-project/textures/sand.png");
    terrainGrassTexture = loadTexture("../Adv-project/textures/grass.png");
    terrainRockTexture = loadTexture("../Adv-project/textures/rock.png");
    terrainSnowTexture = loadTexture("../Adv-project/textures/snow.png");

    // Generate the terrain tile mesh, this will be used for all terrain tiles and the vertex shader will displace it based on the world position of the tile and the procedural height function
    createTerrainTileMesh();

    // Create the Sun
    g_sunSphere = createSphereMesh(1.0f, 50, 50); // low res sphere for sun

    // Spawn the camera above the middle tile so the terrain is clearly in view on frame one.
    cameraPosition = glm::vec3(256.0f, 160.0f, 420.0f);
}

void display()
{
    int w = 0, h = 0;
    SDL_GetWindowSize(g_window, &w, &h);

    const float aspect = float(w) / float(h ? h : 1);
    const mat4 projection = perspective(radians(60.0f), aspect, 0.1f, 3000.0f);

    constexpr int terrainSquareSize = SQUARE_SIZE;
    const int cameraGridX = static_cast<int>(std::floor(cameraPosition.x / float(terrainSquareSize)));
    const int cameraGridZ = static_cast<int>(std::floor(cameraPosition.z / float(terrainSquareSize)));

    // ---- Build Light view/projection matrix ----
    const vec3 shadowTarget(
        (cameraGridX + 0.5f) * terrainSquareSize,
        0.0f,
        (cameraGridZ + 0.5f) * terrainSquareSize
    );

    const float shadowDistance = 800.0f;
    const vec3 lightPos = shadowTarget - normalize(lightDirWorld) * shadowDistance;

    lightViewMatrix = lookAt(lightPos, shadowTarget, vec3(0.0f, 1.0f, 0.0f));

    const float shadowExtent = 500.0f;
    lightProjectionMatrix = ortho(
        -shadowExtent, shadowExtent,
        -shadowExtent, shadowExtent,
        1.0f, 2000.0f
    );

    // ---- Camera ----
    const float totalYaw = cameraYaw + viewYawOffset;
    const float totalPitch = clamp(cameraPitch + viewPitchOffset, radians(-89.0f), radians(89.0f));
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

    labhelper::setUniformSlow(shadowProgram, "lightViewProj", lightProjectionMatrix * lightViewMatrix);
    
    if (!guiSettings.uploadGuard) {
    labhelper::setUniformSlow(shadowProgram, "heightScale", guiSettings.heightMapScale);
    labhelper::setUniformSlow(shadowProgram, "tileHeight", sampleSquare.height);
    labhelper::setUniformSlow(shadowProgram, "tileSeed", terrainTileSeed);
    }
    
    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(2.0f, 4.0f);

    glBindVertexArray(terrainTileVao);

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

            glDrawElements(GL_TRIANGLES, terrainTileIndexCount, GL_UNSIGNED_INT, 0);
        }
    }

    glDisable(GL_POLYGON_OFFSET_FILL);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // ------------------------------------------------------------------------
    // Main pass
    // ------------------------------------------------------------------------
    glViewport(0, 0, w, h);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glUseProgram(terrainShader);

    if (!guiSettings.uploadGuard)
    {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, terrainWaterTexture);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, terrainSandTexture);
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, terrainGrassTexture);
        glActiveTexture(GL_TEXTURE3);
        glBindTexture(GL_TEXTURE_2D, terrainRockTexture);
        glActiveTexture(GL_TEXTURE4);
        glBindTexture(GL_TEXTURE_2D, terrainSnowTexture);

        labhelper::setUniformSlow(terrainShader, "terrainWaterTex", 0);
        labhelper::setUniformSlow(terrainShader, "terrainSandTex", 1);
        labhelper::setUniformSlow(terrainShader, "terrainGrassTex", 2);
        labhelper::setUniformSlow(terrainShader, "terrainRockTex", 3);
        labhelper::setUniformSlow(terrainShader, "terrainSnowTex", 4);

        guiSettings.sendToShader(terrainShader);

        labhelper::setUniformSlow(terrainShader, "shadowsEnabled", guiSettings.shadowsEnabled);
        labhelper::setUniformSlow(terrainShader, "point_light_intensity_multiplier", guiSettings.lightIntensity);

        labhelper::setUniformSlow(terrainShader, "terrainTextureScale", guiSettings.terrainTextureScale);
        labhelper::setUniformSlow(terrainShader, "blend", guiSettings.blend);

        labhelper::setUniformSlow(terrainShader, "wireframeMode", guiSettings.wireframeMode);

        labhelper::setUniformSlow(terrainShader, "heightScale", guiSettings.heightMapScale);
        labhelper::setUniformSlow(terrainShader, "tileWidth", sampleSquare.width);
        labhelper::setUniformSlow(terrainShader, "tileHeight", sampleSquare.height);
        labhelper::setUniformSlow(terrainShader, "tileSeed", terrainTileSeed);

        guiSettings.uploadGuard = true;
    }

    labhelper::setUniformSlow(terrainShader, "lightViewProj", lightProjectionMatrix * lightViewMatrix);
    labhelper::setUniformSlow(terrainShader, "lightDirWorld", lightDirWorld);
    labhelper::setUniformSlow(terrainShader, "cameraPosWorld", cameraPosition);

    glActiveTexture(GL_TEXTURE5);
    glBindTexture(GL_TEXTURE_2D, shadowMap.depthTex);
    labhelper::setUniformSlow(terrainShader, "shadowMap", 5);

    if (guiSettings.wireframeMode)
    {
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    }

    glBindVertexArray(terrainTileVao);

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

            glDrawElements(GL_TRIANGLES, terrainTileIndexCount, GL_UNSIGNED_INT, 0);
        }
    }

    if (guiSettings.wireframeMode)
    {
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }

    // ------------------------------------------------------------------------
    // Draw the Sun
    // ------------------------------------------------------------------------
    glUseProgram(sunProgram);

    vec3 sunPosWorld = -normalize(lightDirWorld) * sunDistance;

    mat4 sunModel(1.0f);
    sunModel = translate(sunModel, sunPosWorld);
    sunModel = scale(sunModel, vec3(sunRadiusWorld));

    mat4 sunMvp = projection * view * sunModel;
    labhelper::setUniformSlow(sunProgram, "mvpMatrix", sunMvp);
    labhelper::setUniformSlow(sunProgram, "sunColor",
                              vec3(1.0f, 1.0f, 0.8f) * guiSettings.lightIntensity);

    glBindVertexArray(g_sunSphere.vao);
    glDrawElements(GL_TRIANGLES, g_sunSphere.indexCount, GL_UNSIGNED_INT, 0);

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
        updateSunFromKeyboard(deltaTimeSeconds);

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
