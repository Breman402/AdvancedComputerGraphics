#version 420
precision highp float;

struct Material
{
    vec3 color;
    float metalness;
    float fresnel;
    float shininess;
    vec3 emission;
};

// ----------------------------------------------------------------------------
// Inputs from vertex shader
// (HEIGHTMAP DISPLACEMENT IS ALREADY APPLIED IN earth.vert)
// ----------------------------------------------------------------------------
in vec2 vTexcoord;       // UV for color lookup
in vec3 vWorldPos;       // world-space position AFTER heightmap displacement
in vec3 vWorldNormal;    // world-space normal AFTER heightmap displacement
in vec4 vShadowCoord;    // position in light clip space (for shadow map)

// ----------------------------------------------------------------------------
// Output
// ----------------------------------------------------------------------------
layout(location = 0) out vec4 fragmentColor;

// ----------------------------------------------------------------------------
// Textures
// ----------------------------------------------------------------------------
uniform sampler2D earthTex;   // Color texture (daytime map)
uniform sampler2D shadowMap;  // Depth texture from shadow pass

// ----------------------------------------------------------------------------
// Lighting uniforms (world space)
// ----------------------------------------------------------------------------
uniform vec3 lightDirWorld;   // Direction *towards* the light (sun), normalized
uniform vec3 cameraPosWorld;  // Camera position in world space

// ----------------------------------------------------------------------------
// Material parameters (SHADING)
// ----------------------------------------------------------------------------
uniform Material earthMaterial;


// Simple light parameters
//uniform vec3  point_light_color = vec3(1.0, 1.0, 1.0); // White light
uniform vec3  point_light_color = vec3(1.0, 0.956, 0.839); // Slightly warm light
uniform float point_light_intensity_multiplier = 1.0;

// Wireframe mode
uniform bool wireframeMode = false;
uniform bool showTexture = true;

#define PI 3.14159265359

// ----------------------------------------------------------------------------
// SHADING: Direct illumination using Torrance–Sparrow BRDF
// (HEIGHTMAP only affects this via vWorldPos / vWorldNormal)
// ----------------------------------------------------------------------------
vec3 calculateDirectIllumination(vec3 wo, vec3 n, vec3 base_color)
{
    // Directional light: same radiance everywhere, no distance attenuation
    // We interpret lightDirWorld as direction *towards* the light,
    // so wi = direction from surface to light = -lightDirWorld.
    vec3 wi = normalize(-lightDirWorld);

    float NdotL = dot(n, wi);
    if (NdotL <= 0.0)
        return vec3(0.0);

    // Radiance from the "sun"
    vec3 Li = point_light_intensity_multiplier * point_light_color;

    // Diffuse term (Lambert)
    vec3 diffuse_term = base_color * (1.0 / PI) * NdotL * Li; // Don't need to do the max here cause of the if statement above

    // Torrance–Sparrow BRDF (specular)
    vec3 wh = normalize(wi + wo); // half vector

    float F_wi = earthMaterial.fresnel +
                 (1.0 - earthMaterial.fresnel) *
                 pow(1.0 - max(0.01, dot(wh, wi)), 5.0); // Fresnel term

    float D_wh = (earthMaterial.shininess + 2.0) / (2.0 * PI) *
                 pow(max(0.01, dot(n, wh)), earthMaterial.shininess); // NDF

    float G_1 = 2.0 * dot(n, wh) * dot(n, wo) / dot(wo, wh);
    float G_2 = 2.0 * dot(n, wh) * dot(n, wi) / dot(wo, wh);
    float G   = min(1.0, min(G_1, G_2)); // Geometry term

    float denom = 4.0 * clamp(dot(n, wo) * dot(n, wi), 0.001, 1.0);
    float brdf  = (F_wi * D_wh * G) / denom;

    float cosTheta = max(dot(n, wi), 0.0);

    // Dielectric vs metal behavior
    vec3 dielectric_term = brdf * cosTheta * Li +
                           (1.0 - F_wi) * diffuse_term;

    vec3 metal_term = brdf * base_color * cosTheta * Li;

    vec3 direct_illum =
        earthMaterial.metalness * metal_term +
        (1.0 - earthMaterial.metalness) * dielectric_term;

    return direct_illum;
}

void main()
{
    // ============================================================================
    // WIREFRAME MODE: Skip all shading and just output wireframe color
    // ============================================================================
    if (wireframeMode)
    {
        fragmentColor = vec4(0.8, 0.8, 0.8, 1.0); // Light gray wireframe
        return;
    }

    // ============================================================================
    // HEIGHTMAP NOTE:
    //
    // The heightmap displacement is handled entirely in earth.vert, which
    // changes vWorldPos and vWorldNormal. Here we just use those as inputs
    // for shadows and shading.
    // ============================================================================

    // View direction (world space): from surface point to camera
    vec3 wo = normalize(cameraPosWorld - vWorldPos);
    vec3 n  = normalize(vWorldNormal);

    // Base color from Earth texture, modulated by the globe's material_color
    vec3 base_color;
    if (showTexture)
    {
        base_color = texture(earthTex, vTexcoord).rgb * earthMaterial.color;
    }
    else
    {
        base_color = earthMaterial.color;
    }

    // ============================================================================
    // SHADOW HANDLING: sample shadow map with PCF and compute shadow factor
    // ============================================================================
    // vShadowCoord is in light clip space; we need to do perspective divide
    // and remap from [-1,1] to [0,1] to sample the shadowMap.
    vec3 projCoords = vShadowCoord.xyz / vShadowCoord.w;
    projCoords = projCoords * 0.5 + 0.5; // from [-1,1] to [0,1]

    // Default: no shadow
    float shadow = 0.0;

    // Only sample shadow map if inside light frustum
    if (projCoords.x >= 0.0 && projCoords.x <= 1.0 &&
        projCoords.y >= 0.0 && projCoords.y <= 1.0 &&
        projCoords.z >= 0.0 && projCoords.z <= 1.0)
    {
        float currentDepth = projCoords.z; // this fragment depth
        float bias = 0.001;

        // PCF: sample multiple points around the texel for smoother shadows
        // This will show improvement with higher shadow map resolutions
        float texelSize = 1.0 / textureSize(shadowMap, 0).x;
        for (int x = -2; x <= 2; ++x)
        {
            for (int y = -2; y <= 2; ++y)
            {
                float closestDepth = texture(shadowMap, projCoords.xy + vec2(x, y) * texelSize).r;
                shadow += currentDepth - bias > closestDepth ? 0.04 : 0.0; // 25 samples total
            }
        }
    }

    // ============================================================================
    // SHADING: direct illumination (modulated by shadow) + optional emission
    // ============================================================================
    vec3 direct_illumination_term =
        (1.0 - shadow) * calculateDirectIllumination(wo, n, base_color);

    // No indirect environment lighting for now
    vec3 indirect_illumination_term = vec3(0.0);

    // Emission term set to 0 for now
    vec3 emission_term = earthMaterial.emission;

    vec3 shading = direct_illumination_term +
                   indirect_illumination_term +
                   emission_term;

    fragmentColor = vec4(shading, 1.0);
}
