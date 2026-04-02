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

in vec2 vTexcoord;
in vec3 vWorldPos;
in vec3 vWorldNormal;
in vec4 vShadowCoord;
in float vHeight;

layout(location = 0) out vec4 fragmentColor;

uniform sampler2D terrainWaterTex;
uniform sampler2D terrainSandTex;
uniform sampler2D terrainGrassTex;
uniform sampler2D terrainRockTex;
uniform sampler2D terrainSnowTex;

uniform float tileHeight;
uniform float terrainTextureScale;
uniform float blend;

uniform sampler2D shadowMap;
uniform bool shadowsEnabled = false;

uniform vec3 lightDirWorld;
uniform vec3 cameraPosWorld;

uniform Material waterMaterial;
uniform Material sandMaterial;
uniform Material grassMaterial;
uniform Material rockMaterial;
uniform Material snowMaterial;

uniform vec3  point_light_color = vec3(1.0, 0.956, 0.839);
uniform float point_light_intensity_multiplier = 1.0;

uniform bool wireframeMode = false;
uniform bool showTexture = true;

#define PI 3.14159265359

vec3 calculateDirectIllumination(vec3 wo, vec3 n, vec3 base_color, Material mat)
{
    vec3 wi = normalize(-lightDirWorld);

    float NdotL = dot(n, wi);
    if (NdotL <= 0.0)
        return vec3(0.0);

    vec3 Li = point_light_intensity_multiplier * point_light_color;
    vec3 diffuse_term = base_color * (1.0 / PI) * NdotL * Li;

    vec3 wh = normalize(wi + wo);

    float F_wi = mat.fresnel +
                 (1.0 - mat.fresnel) *
                 pow(1.0 - max(0.01, dot(wh, wi)), 5.0);

    float D_wh = (mat.shininess + 2.0) / (2.0 * PI) *
                 pow(max(0.01, dot(n, wh)), mat.shininess);

    float G_1 = 2.0 * dot(n, wh) * dot(n, wo) / dot(wo, wh);
    float G_2 = 2.0 * dot(n, wh) * dot(n, wi) / dot(wo, wh);
    float G   = min(1.0, min(G_1, G_2));

    float denom = 4.0 * clamp(dot(n, wo) * dot(n, wi), 0.001, 1.0);
    float brdf  = (F_wi * D_wh * G) / denom;

    float cosTheta = max(dot(n, wi), 0.0);

    vec3 dielectric_term = brdf * cosTheta * Li +
                           (1.0 - F_wi) * diffuse_term;

    vec3 metal_term = brdf * base_color * cosTheta * Li;

    return mat.metalness * metal_term +
           (1.0 - mat.metalness) * dielectric_term;
}

void main()
{
    if (wireframeMode)
    {
        fragmentColor = vec4(0.8, 0.8, 0.8, 1.0);
        return;
    }

    vec3 wo = normalize(cameraPosWorld - vWorldPos);
    vec3 n  = normalize(vWorldNormal);

    float h = clamp(vHeight / tileHeight, 0.0, 1.0);
    vec2 tiledUV = vWorldPos.xz * terrainTextureScale;

    vec3 water = texture(terrainWaterTex, tiledUV).rgb;
    vec3 sand  = texture(terrainSandTex,  tiledUV).rgb;
    vec3 grass = texture(terrainGrassTex, tiledUV).rgb;
    vec3 rock  = texture(terrainRockTex,  tiledUV).rgb;
    vec3 snow  = texture(terrainSnowTex,  tiledUV).rgb;

    float waterToSand = smoothstep(0.03 - blend, 0.03 + blend, h);
    float sandToGrass = smoothstep(0.06 - blend, 0.06 + blend, h);
    float grassToRock = smoothstep(0.30 - blend, 0.30 + blend, h);
    float rockToSnow  = smoothstep(0.50 - blend, 0.50 + blend, h);

    vec3 base_color;
    if (showTexture)
    {
        vec3 col = mix(water, sand, waterToSand);
        col = mix(col, grass, sandToGrass);
        col = mix(col, rock, grassToRock);
        col = mix(col, snow, rockToSnow);
        base_color = col;
    }
    else
    {
        vec3 colorTint = mix(waterMaterial.color, sandMaterial.color, waterToSand);
        colorTint = mix(colorTint, grassMaterial.color, sandToGrass);
        colorTint = mix(colorTint, rockMaterial.color, grassToRock);
        colorTint = mix(colorTint, snowMaterial.color, rockToSnow);
        base_color = colorTint;
    }

    float metalness = mix(waterMaterial.metalness, sandMaterial.metalness, waterToSand);
    metalness = mix(metalness, grassMaterial.metalness, sandToGrass);
    metalness = mix(metalness, rockMaterial.metalness, grassToRock);
    metalness = mix(metalness, snowMaterial.metalness, rockToSnow);

    float fresnel = mix(waterMaterial.fresnel, sandMaterial.fresnel, waterToSand);
    fresnel = mix(fresnel, grassMaterial.fresnel, sandToGrass);
    fresnel = mix(fresnel, rockMaterial.fresnel, grassToRock);
    fresnel = mix(fresnel, snowMaterial.fresnel, rockToSnow);

    float shininess = mix(waterMaterial.shininess, sandMaterial.shininess, waterToSand);
    shininess = mix(shininess, grassMaterial.shininess, sandToGrass);
    shininess = mix(shininess, rockMaterial.shininess, grassToRock);
    shininess = mix(shininess, snowMaterial.shininess, rockToSnow);

    vec3 emission = mix(waterMaterial.emission, sandMaterial.emission, waterToSand);
    emission = mix(emission, grassMaterial.emission, sandToGrass);
    emission = mix(emission, rockMaterial.emission, grassToRock);
    emission = mix(emission, snowMaterial.emission, rockToSnow);

    Material blendedMaterial;
    blendedMaterial.color = vec3(1.0);
    blendedMaterial.metalness = metalness;
    blendedMaterial.fresnel = fresnel;
    blendedMaterial.shininess = shininess;
    blendedMaterial.emission = emission;

    vec3 projCoords = vShadowCoord.xyz / vShadowCoord.w;
    projCoords = projCoords * 0.5 + 0.5;

    float shadow = 0.0;
    if (shadowsEnabled &&
        projCoords.x >= 0.0 && projCoords.x <= 1.0 &&
        projCoords.y >= 0.0 && projCoords.y <= 1.0 &&
        projCoords.z >= 0.0 && projCoords.z <= 1.0)
    {
        float currentDepth = projCoords.z;
        float bias = 0.001;

        float texelSize = 1.0 / textureSize(shadowMap, 0).x;
        for (int x = -2; x <= 2; ++x)
        {
            for (int y = -2; y <= 2; ++y)
            {
                float closestDepth = texture(shadowMap, projCoords.xy + vec2(x, y) * texelSize).r;
                shadow += currentDepth - bias > closestDepth ? 0.04 : 0.0;
            }
        }
    }

    vec3 direct_illumination_term =
        (1.0 - shadow) * calculateDirectIllumination(wo, n, base_color, blendedMaterial);

    vec3 indirect_illumination_term = vec3(0.0);
    vec3 emission_term = blendedMaterial.emission;

    vec3 shading = direct_illumination_term +
                   indirect_illumination_term +
                   emission_term;

    fragmentColor = vec4(shading, 1.0);
}