#version 420 core

in vec3 vWorldNormal;
in vec3 vWorldPos;
in vec4 vShadowCoord;
in vec2 vTexCoord;
flat in int vTerrainAllowed;

out vec4 fragmentColor;

uniform bool has_color_texture;
uniform sampler2D color_texture;
uniform vec3 material_color;

uniform sampler2D shadowMap;
uniform bool shadowsEnabled;

uniform vec3 lightDirWorld;
uniform vec3 cameraPosWorld;
uniform vec3 point_light_color = vec3(1.0, 0.956, 0.839);
uniform float point_light_intensity_multiplier = 1.0;
uniform vec3 skyAmbientColor = vec3(0.03, 0.05, 0.08);
uniform float skyAmbientStrength = 0.18;
uniform vec3 atmosphereFogColor = vec3(0.04, 0.06, 0.08);
uniform float atmosphereFogDensity = 0.001;

const float PI = 3.14159265359;

float calculateShadow(vec3 normal, vec3 lightToSurfaceDir)
{
    vec3 projCoords = vShadowCoord.xyz / vShadowCoord.w;
    projCoords = projCoords * 0.5 + 0.5;

    if (!shadowsEnabled ||
        projCoords.x < 0.0 || projCoords.x > 1.0 ||
        projCoords.y < 0.0 || projCoords.y > 1.0 ||
        projCoords.z < 0.0 || projCoords.z > 1.0)
    {
        return 0.0;
    }

    float currentDepth = projCoords.z;
    float bias = max(0.0015 * (1.0 - dot(normal, lightToSurfaceDir)), 0.0004);
    float shadow = 0.0;
    float texelSize = 1.0 / textureSize(shadowMap, 0).x;

    for (int x = -2; x <= 2; ++x)
    {
        for (int y = -2; y <= 2; ++y)
        {
            float closestDepth = texture(shadowMap, projCoords.xy + vec2(x, y) * texelSize).r;
            shadow += currentDepth - bias > closestDepth ? 0.04 : 0.0;
        }
    }

    return shadow;
}

void main()
{
    if (vTerrainAllowed == 0)
    {
        discard;
    }

    vec3 baseColor = material_color;
    float alpha = 1.0;
    if (has_color_texture)
    {
        vec4 texel = texture(color_texture, vTexCoord);
        if (texel.a < 0.35)
        {
            discard;
        }
        baseColor *= texel.rgb;
        alpha = texel.a;
    }

    vec3 normal = normalize(vWorldNormal);
    vec3 wi = normalize(-lightDirWorld);
    float diffuse = max(dot(normal, wi), 0.0);
    float shadow = calculateShadow(normal, wi);

    vec3 directLight = baseColor * point_light_color * point_light_intensity_multiplier * (1.0 / PI) * diffuse;
    directLight *= (1.0 - shadow);

    float upFacing = clamp(normal.y * 0.5 + 0.5, 0.0, 1.0);
    vec3 ambientLight = baseColor * skyAmbientColor * skyAmbientStrength * mix(0.45, 1.0, upFacing);
    vec3 color = directLight + ambientLight;

    vec3 wo = normalize(cameraPosWorld - vWorldPos);
    float viewDistance = length(cameraPosWorld - vWorldPos);
    float horizonView = 1.0 - clamp(abs(wo.y), 0.0, 1.0);
    float fogAmount = 1.0 - exp(-viewDistance * atmosphereFogDensity);
    fogAmount *= mix(0.35, 1.0, horizonView);
    color = mix(color, atmosphereFogColor, clamp(fogAmount, 0.0, 0.70));

    fragmentColor = vec4(color, alpha);
}
