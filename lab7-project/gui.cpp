#include <imgui.h>
#include <imgui_impl_sdl_gl3.h>
#include <cstdio>
#include "gui.hpp"

void gui(SphereMesh& sphere, float& heightMapScale, int& shadowMapSize, ShadowMap& shadowMap, Material& GlobeMaterial, bool& wireframeMode, bool& showTexture, EarthTextureToUse& earthTextureOption)
{

    // ----------------- Save default values --------------------
    static const float defaultHeightMapScale = heightMapScale;
    static const int   defaultShadowMapSize  = shadowMapSize;
    static const float defaultSphereRadius   = sphere.radius;
    static const int   defaultSphereStacks   = sphere.stacks;
    static const int   defaultSphereSlices   = sphere.slices;
    static const Material defaultGlobeMaterial = GlobeMaterial;
    static const bool  defaultWireframeMode  = wireframeMode;
    static const bool  defaultShowTexture    = showTexture;

	// ----------------- Set variables --------------------------
	ImGui::Text("Sphere Properties");
    ImGui::SliderFloat("Sphere Radius", &sphere.radius, 0.1f, 10.0f);
	ImGui::SliderInt("Stacks", &sphere.stacks, 3, 5000);
    ImGui::SliderInt("Slices", &sphere.slices, 3, 5000);
	if (ImGui::Button("Apply##sphere")) {
        printf("Updating sphere mesh: radius=%.2f, stacks=%d, slices=%d\n",
               sphere.radius, sphere.stacks, sphere.slices);
        updateSphereMesh(sphere);
        printf("Sphere mesh updated.\n");
    }

    ImGui::Separator();

    ImGui::Text("Height Map Properties");
    ImGui::SliderFloat("Height Map Scale", &heightMapScale, 0.0f, 1.0f);

    ImGui::Separator();

    ImGui::Text("Shadow Map Properties");
    ImGui::SliderInt("Shadow Map Size", &shadowMapSize, 2, 8192);
    if (ImGui::Button("Apply##shadowmap")) {
        // Update shadow map size
        printf("Updating shadow map size to %d\n", shadowMapSize);
        updateShadowMap(shadowMap, shadowMapSize);
        printf("Shadow map updated.\n");
    }

    ImGui::Separator();
    
    ImGui::Text("Material Properties of the Earth");
    ImGui::SliderFloat("Metalness", &GlobeMaterial.metalness, 0.0f, 1.0f);
    ImGui::SliderFloat("Fresnel", &GlobeMaterial.fresnel, 0.0f, 1.0f);
    ImGui::SliderFloat("Shininess", &GlobeMaterial.shininess, 1.0f, 256.0f);
    ImGui::ColorEdit3("Emission", (float*)&GlobeMaterial.emission);

    ImGui::Separator();

    ImGui::Checkbox("Wireframe Mode", &wireframeMode);
    ImGui::Checkbox("Show Texture", &showTexture);

    ImGui::Separator();

    ImGui::Text("Earth Texture Options");
    if (ImGui::Checkbox("Use Blue Marble 8k PNG Texture", &earthTextureOption.useBlueMarbel8kPNG)) {
        earthTextureOption.useMapMode = false; // these are here to make sure that there is mutual exclusivity
        earthTextureOption.useGrayScaleHeightMap = false;
    }
    if (ImGui::Checkbox("Use Maplike Texture", &earthTextureOption.useMapMode)) {
       earthTextureOption.useBlueMarbel8kPNG = false;
       earthTextureOption.useGrayScaleHeightMap = false;
    }
    if (ImGui::Checkbox("Use Grayscale Height Map", &earthTextureOption.useGrayScaleHeightMap)) {
        earthTextureOption.useMapMode = false;
        earthTextureOption.useBlueMarbel8kPNG = false;
    }

    ImGui::Separator();

    if (ImGui::Button("Reset to Default Values")) {
        // Reset all parameters to their default values
        heightMapScale = defaultHeightMapScale;
        shadowMapSize  = defaultShadowMapSize;
        sphere.radius  = defaultSphereRadius;
        sphere.stacks  = defaultSphereStacks;
        sphere.slices  = defaultSphereSlices;
        GlobeMaterial = defaultGlobeMaterial;
        wireframeMode  = defaultWireframeMode;
        showTexture    = defaultShowTexture;

        // Update sphere mesh and shadow map to reflect reset values
        updateSphereMesh(sphere);
        updateShadowMap(shadowMap, shadowMapSize);
    }
    if (ImGui::Button("Use best looking values"))
    {
        sphere.stacks = 5000;
        sphere.slices = 5000;
        heightMapScale = 0.014f;
        shadowMapSize = 4096;
        GlobeMaterial = defaultGlobeMaterial;
        updateSphereMesh(sphere);
        updateShadowMap(shadowMap, shadowMapSize);
    }

    ImGui::Separator();

	ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate,
	            ImGui::GetIO().Framerate);
	// ----------------------------------------------------------
}

