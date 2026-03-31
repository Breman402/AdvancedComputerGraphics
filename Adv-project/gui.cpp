#include <imgui.h>
#include <imgui_impl_sdl_gl3.h>
#include <cstdio>
#include "gui.hpp"

void gui(float* lightIntensity, float* heightMapScale, float* terrainTextureScale, float* blend, float* cameraMoveSpeed, std::vector<Material*>* materials, bool* wireframeMode, bool* uploadGuard)
{
    ImGui::Begin("Settings");
    
    ImGui::Separator();

    ImGui::Text("Camera:");
    ImGui::SliderFloat("Movement Speed", cameraMoveSpeed, 1.0f, 1000.0f);

    ImGui::Separator();

    ImGui::Text("Lighting:");
    if (ImGui::SliderFloat("Light Intensity", lightIntensity, 0.0f, 100.0f))
        *uploadGuard = false;
    
    ImGui::Separator();
    
    ImGui::Text("Terrain:");
    if (ImGui::SliderFloat("Height Map Scale", heightMapScale, 0.0f, 100.0f))
        *uploadGuard = false;
    if (ImGui::SliderFloat("Terrain Texture Scale", terrainTextureScale, 0.0f, 10.0f))
        *uploadGuard = false;
    if (ImGui::SliderFloat("Blend", blend, 0.0f, 1.0f))
        *uploadGuard = false;

    ImGui::Separator();

    ImGui::Text("Materials:");
    for (size_t i = 0; i < materials->size(); ++i)
    {
        Material& mat = *(*materials)[i];
        std::string header = "Material " + std::to_string(i);
        if (ImGui::CollapsingHeader(header.c_str()))
        {
            if (ImGui::ColorEdit3(("Color##" + std::to_string(i)).c_str(), &mat.color[0]))
                *uploadGuard = false;
            if (ImGui::SliderFloat(("Metalness##" + std::to_string(i)).c_str(), &mat.metalness, 0.0f, 1.0f))
                *uploadGuard = false;
            if (ImGui::SliderFloat(("Fresnel##" + std::to_string(i)).c_str(), &mat.fresnel, 0.0f, 1.0f))
                *uploadGuard = false;
            if (ImGui::SliderFloat(("Shininess##" + std::to_string(i)).c_str(), &mat.shininess, 1.0f, 256.0f))
                *uploadGuard = false;
            if (ImGui::ColorEdit3(("Emission##" + std::to_string(i)).c_str(), &mat.emission[0]))
                *uploadGuard = false;
        }
    }

    ImGui::Separator();
    if (ImGui::Checkbox("Wireframe Mode", wireframeMode))
        *uploadGuard = false;

    ImGui::End();
}

