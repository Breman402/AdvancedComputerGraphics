#version 420 core

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

// Terrain textures
uniform sampler2D terrainWaterTex;
uniform sampler2D terrainSandTex;
uniform sampler2D terrainGrassTex0;
uniform sampler2D terrainGrassTex1;
uniform sampler2D terrainGrassTex2;
uniform sampler2D terrainGrassTex3;
uniform sampler2D terrainRockTex0;
uniform sampler2D terrainRockTex1;
uniform sampler2D terrainRockTex2;
uniform sampler2D terrainRockTex3;
uniform sampler2D terrainSnowTex0;
uniform sampler2D terrainSnowTex1;
uniform sampler2D terrainSnowTex2;
uniform sampler2D terrainSnowTex3;

// Terrain material properties
uniform Material waterMaterial;
uniform Material sandMaterial;
uniform Material grassMaterial;
uniform Material rockMaterial;
uniform Material snowMaterial;

// Terrain Texture parameters
uniform float terrainTextureScale;
uniform float blend;
const float grassBlendPatchScale = 0.045;
const float rockBlendPatchScale = 0.035;
const float snowBlendPatchScale = 0.030;

uniform float tileHeight; // Maximum height for terrain tiling calculations


uniform sampler2D shadowMap;
uniform bool shadowsEnabled;

uniform vec3 lightDirWorld;
uniform vec3 cameraPosWorld;

// Sky and atmosphere parameters
uniform vec3 skyAmbientColor = vec3(0.03, 0.05, 0.08);
uniform float skyAmbientStrength = 0.18;
uniform vec3 atmosphereFogColor = vec3(0.04, 0.06, 0.08);
uniform float atmosphereFogDensity = 0.001;

// Point light parameters
uniform vec3  point_light_color = vec3(1.0, 0.956, 0.839);
uniform float point_light_intensity_multiplier = 1.0;

uniform bool wireframeMode = false;

#define PI 3.14159265359
// ----------------------------------------------------------------------------
// Hash and noise functions for procedural texture blending
// ----------------------------------------------------------------------------
float hash(vec2 p)
{
    return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453123);
}

float valueNoise(vec2 p)
{
    vec2 i = floor(p);
    vec2 f = fract(p);

    float a = hash(i);
    float b = hash(i + vec2(1.0, 0.0));
    float c = hash(i + vec2(0.0, 1.0));
    float d = hash(i + vec2(1.0, 1.0));

    vec2 u = f * f * (3.0 - 2.0 * f);

    return mix(
        mix(a, b, u.x),
        mix(c, d, u.x),
        u.y
    );
}
// ----------------------------------------------------------------------------
/// Procedural noise weights for more natural texture blending patterns
// ----------------------------------------------------------------------------
vec4 textureBlendWeights(vec2 worldXZ, float patchScale, vec2 seedOffset)
{
    vec2 p = worldXZ * patchScale + seedOffset;
    vec4 weights = vec4(
        valueNoise(p + vec2(0.0, 0.0)),
        valueNoise(p + vec2(37.2, 11.8)),
        valueNoise(p + vec2(83.4, 51.7)),
        valueNoise(p + vec2(19.6, 94.3))
    );

    weights = pow(weights, vec4(4.0)) + vec4(0.02);
    return weights / dot(weights, vec4(1.0));
}

vec4 monotonicHeightBias(float t)
{
    return vec4(
        mix(1.6, 0.4, t),
        mix(1.2, 0.8, t),
        mix(0.8, 1.2, t),
        mix(0.4, 1.6, t)
    );
}

vec3 sampleGrass(vec2 tiledUV, vec2 worldXZ)
{
    vec4 weights = textureBlendWeights(worldXZ, grassBlendPatchScale, vec2(0.0, 0.0));
    vec3 grass0 = texture(terrainGrassTex0, tiledUV).rgb;
    vec3 grass1 = texture(terrainGrassTex1, tiledUV).rgb;
    vec3 grass2 = texture(terrainGrassTex2, tiledUV).rgb;
    vec3 grass3 = texture(terrainGrassTex3, tiledUV).rgb;

    return grass0 * weights.x +
           grass1 * weights.y +
           grass2 * weights.z +
           grass3 * weights.w;
}

vec3 sampleRock(vec2 tiledUV, vec2 worldXZ, float rockHeight01)
{
    vec4 weights = textureBlendWeights(worldXZ, rockBlendPatchScale, vec2(121.3, 48.9));
    weights *= monotonicHeightBias(rockHeight01);
    weights /= dot(weights, vec4(1.0));

    vec3 rock0 = texture(terrainRockTex0, tiledUV).rgb;
    vec3 rock1 = texture(terrainRockTex1, tiledUV).rgb;
    vec3 rock2 = texture(terrainRockTex2, tiledUV).rgb;
    vec3 rock3 = texture(terrainRockTex3, tiledUV).rgb;

    return rock0 * weights.x +
           rock1 * weights.y +
           rock2 * weights.z +
           rock3 * weights.w;
}

vec3 sampleSnow(vec2 tiledUV, vec2 worldXZ, float snowHeight01)
{
    vec4 weights = textureBlendWeights(worldXZ, snowBlendPatchScale, vec2(67.5, 173.2));
    weights *= monotonicHeightBias(snowHeight01);
    weights /= dot(weights, vec4(1.0));

    vec3 snow0 = texture(terrainSnowTex0, tiledUV).rgb;
    vec3 snow1 = texture(terrainSnowTex1, tiledUV).rgb;
    vec3 snow2 = texture(terrainSnowTex2, tiledUV).rgb;
    vec3 snow3 = texture(terrainSnowTex3, tiledUV).rgb;

    return snow0 * weights.x +
           snow1 * weights.y +
           snow2 * weights.z +
           snow3 * weights.w;
}
// ----------------------------------------------------------------------------

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
    float G = max(0.0, min(1.0, min(G_1, G_2)));

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
    vec2 tiledUV = vWorldPos.xz * terrainTextureScale; // World position scaled for texturing

    float rockHeight01 = clamp((h - 0.25) / (0.50 - 0.25), 0.0, 1.0); // Normalized height for rock texture blending
    float snowHeight01 = clamp((h - 0.50) / (1.00 - 0.50), 0.0, 1.0);

    vec3 water = texture(terrainWaterTex, tiledUV).rgb;
    vec3 sand  = texture(terrainSandTex,  tiledUV).rgb;
    vec3 grass = sampleGrass(tiledUV, vWorldPos.xz); // Pick one of the grass textures
    vec3 rock  = sampleRock(tiledUV, vWorldPos.xz, rockHeight01); // Pick one of the rock textures
    vec3 snow  = sampleSnow(tiledUV, vWorldPos.xz, snowHeight01); // Pick one of the snow textures

    float waterToSand = smoothstep(0.03 - blend, 0.03 + blend, h); // Blend factor for water to sand transition
    float sandToGrass = smoothstep(0.06 - blend, 0.06 + blend, h);
    float grassToRock = smoothstep(0.25 - blend, 0.25 + blend, h);
    float rockToSnow  = smoothstep(0.50 - blend, 0.50 + blend, h);

    vec3 base_color;

    vec3 col = mix(water, sand, waterToSand);
    col = mix(col, grass, sandToGrass);
    col = mix(col, rock, grassToRock);
    col = mix(col, snow, rockToSnow);
    base_color = col;

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

    float upFacing = clamp(n.y * 0.5 + 0.5, 0.0, 1.0);
    vec3 indirect_illumination_term =
        base_color * skyAmbientColor * skyAmbientStrength * mix(0.45, 1.0, upFacing);
    vec3 emission_term = blendedMaterial.emission;

    vec3 shading = direct_illumination_term +
                   indirect_illumination_term +
                   emission_term;

    float viewDistance = length(cameraPosWorld - vWorldPos);
    float horizonView = 1.0 - clamp(abs(wo.y), 0.0, 1.0);
    float fogAmount = 1.0 - exp(-viewDistance * atmosphereFogDensity);
    fogAmount *= mix(0.35, 1.0, horizonView);
    shading = mix(shading, atmosphereFogColor, clamp(fogAmount, 0.0, 0.70));

    fragmentColor = vec4(shading, 1.0);
}
