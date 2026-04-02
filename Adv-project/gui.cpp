#include <imgui.h>
#include <imgui_impl_sdl_gl3.h>
#include <cstdio>
#include <string>
#include "gui.hpp"
#include "createShadow.hpp"

void gui(GUISettings& settings)
{
    const auto markSettingsDirty = [&settings]()
    {
        settings.uploadGuard = false;
    };

    ImGui::Begin("Settings");
    
    ImGui::Separator();

    ImGui::Text("Camera:");
    ImGui::SliderFloat("Movement Speed", &settings.cameraMoveSpeed, 1.0f, 1000.0f);

    ImGui::Separator();

    ImGui::Text("Lighting:");
    if (ImGui::SliderFloat("Light Intensity", &settings.lightIntensity, 0.0f, 100.0f))
        markSettingsDirty();
    
    ImGui::Separator();
    
    ImGui::Text("Terrain:");
    if (ImGui::SliderFloat("Height Map Scale", &settings.heightMapScale, 0.0f, 100.0f))
        markSettingsDirty();
    if (ImGui::SliderFloat("Terrain Texture Scale", &settings.terrainTextureScale, 0.0f, 10.0f))
        markSettingsDirty();
    if (ImGui::SliderFloat("Blend", &settings.blend, 0.0f, 1.0f))
        markSettingsDirty();

    ImGui::Separator();

    ImGui::Text("Materials:");
    for (size_t i = 0; i < settings.materials.size(); ++i)
    {
        Material& mat = *settings.materials[i];
        std::string header = "Material " + std::to_string(i);
        if (ImGui::CollapsingHeader(header.c_str()))
        {
            if (ImGui::ColorEdit3(("Color##" + std::to_string(i)).c_str(), &mat.color[0]))
                markSettingsDirty();
            if (ImGui::SliderFloat(("Metalness##" + std::to_string(i)).c_str(), &mat.metalness, 0.0f, 1.0f))
                markSettingsDirty();
            if (ImGui::SliderFloat(("Fresnel##" + std::to_string(i)).c_str(), &mat.fresnel, 0.0f, 1.0f))
                markSettingsDirty();
            if (ImGui::SliderFloat(("Shininess##" + std::to_string(i)).c_str(), &mat.shininess, 1.0f, 256.0f))
                markSettingsDirty();
            if (ImGui::ColorEdit3(("Emission##" + std::to_string(i)).c_str(), &mat.emission[0]))
                markSettingsDirty();
        }
    }

    ImGui::Separator();
    
    ImGui::Text("Shadows:");
    if (ImGui::Checkbox("Enable Shadows", &settings.shadowsEnabled))
        markSettingsDirty();
    if (ImGui::SliderInt("Shadow Map Size", &settings.shadowMapSize, 512, 8192))
        updateShadowMap(*settings.shadowMap, settings.shadowMapSize);

    ImGui::Separator();
    ImGui::Text("Sun:");
    ImGui::Value("Time of Day Angle", settings.sunTimeOfDayAngle);
    ImGui::SliderFloat("Sun Distance", &settings.sunDistance, 100.0f, 5000.0f);
    ImGui::SliderFloat("Sun Radius World", &settings.sunRadiusWorld, 1.0f, 200.0f);

    ImGui::Separator();
    if (ImGui::Checkbox("Wireframe Mode", &settings.wireframeMode))
        markSettingsDirty();
    ImGui::End();
}

