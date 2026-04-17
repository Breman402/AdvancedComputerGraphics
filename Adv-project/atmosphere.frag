#version 420 core

out vec4 FragColor;

in vec3 vViewDir;

uniform vec3 sunDirectionWorld;
uniform float turbidity;
uniform float rayleigh;
uniform float mieCoefficient;
uniform float mieDirectionalG;
uniform float sunIntensityScale;
uniform float exposure;
uniform float starIntensity;
uniform sampler2D starTexture;

// Constants and magic values
const float PI = 3.14159265359;
const float THREE_OVER_SIXTEENPI = 3.0 / (16.0 * PI);
const float ONE_OVER_FOURPI = 1.0 / (4.0 * PI);
const vec3 totalRayleigh = vec3(5.804543e-6, 1.3562911e-5, 3.0265902e-5);
const float MieConst = 1.8399918514433978e14;
const vec3 MieK = vec3(0.686, 0.678, 0.666);
const float rayleighZenithLength = 8.4e3;
const float mieZenithLength = 1.25e3;
const float sunAngularDiameterCos = 0.9999566769;
const float cutoffAngle = PI / 1.95;
const float steepness = 1.5;
const float EE = 1000.0;
const float TAU = 6.28318530718;

// Convert 3D direction to equirectangular UV coordinates for star texture lookup
vec2 directionToEquirectUV(vec3 d)
{
    d = normalize(d);
    return vec2(
        fract(atan(d.z, d.x) / TAU + 0.5),
        asin(clamp(d.y, -1.0, 1.0)) / PI + 0.5
    );
}

// Calculate sun intensity based on zenith angle. This part controls brightness falloff near horizon
float sunIntensity(float zenithAngleCos)
{
    float angle = acos(clamp(zenithAngleCos, -1.0, 1.0));
    return EE * max(0.0, 1.0 - exp(-(cutoffAngle - angle) / steepness));
}

// Calculate Mie scattering coefficients based on turbidity. This part controls how much light is scattered by particles in the atmosphere, affecting haze and sun halo intensity
vec3 totalMie(float T)
{
    float c = (0.2 * T) * 1.0e-17;
    return 0.434 * c * MieConst * MieK;
}

// Rayleigh phase function. This part describes how light scatters at shorter wavelengths
float rayleighPhase(float cosTheta)
{
    return THREE_OVER_SIXTEENPI * (1.0 + cosTheta * cosTheta);
}

// Henyey-Greenstein phase function. This part describes Mie scattering phase, which is more forward-scattering and affects the appearance of the sun halo and overall sky brightness
float hgPhase(float cosTheta, float g)
{
    float g2 = g * g;
    float denom = pow(max(1.0 - 2.0 * g * cosTheta + g2, 1.0e-3), 1.5);
    return ONE_OVER_FOURPI * ((1.0 - g2) / denom);
}

void main()
{
    // Normalize viewing direction and sun direction
    vec3 direction = normalize(vViewDir);
    vec3 sunDir = normalize(sunDirectionWorld);
    vec3 up = vec3(0.0, 1.0, 0.0);

    // Calculate scattering coefficients for Rayleigh (short wavelengths) and Mie (particles)
    vec3 betaR = totalRayleigh * max(rayleigh, 0.0);
    vec3 betaM = totalMie(max(turbidity, 1.0)) * max(mieCoefficient, 0.0);

    // Calculate sun's intensity based on position
    float sunE = sunIntensity(dot(sunDir, up)) * max(sunIntensityScale, 0.05);

    // Calculate optical depth, which is how much light is attenuated by atmosphere
    float cosZenith = clamp(dot(up, direction), 0.0, 1.0);
    float zenithAngle = acos(cosZenith);
    float inverse = 1.0 / (cos(zenithAngle) + 0.15 * pow(max(93.885 - degrees(zenithAngle), 1.0e-3), -1.253));

    float sR = rayleighZenithLength * inverse;
    float sM = mieZenithLength * inverse;

    // Calculate transmission - how much light remains after scattering
    vec3 Fex = exp(-(betaR * sR + betaM * sM));

    // Calculate phase functions and scattering contribution
    float cosTheta = dot(direction, sunDir);
    vec3 betaRTheta = betaR * rayleighPhase(cosTheta);
    vec3 betaMTheta = betaM * hgPhase(cosTheta, mieDirectionalG);
    vec3 scatteringRatio = (betaRTheta + betaMTheta) / max(betaR + betaM, vec3(1.0e-6));

    // Calculate in-scattered light from sun
    vec3 Lin = sunE * scatteringRatio * (1.0 - Fex);

    // Enhance colors during sunset/sunrise
    float sunsetBoost = clamp(pow(1.0 - max(sunDir.y, 0.0), 5.0), 0.0, 1.0);
    Lin *= mix(
        vec3(1.0),
        pow(max(sunE * scatteringRatio * Fex, vec3(0.0)), vec3(0.5)),
        sunsetBoost
    );

    // Add ambient light and sun disk
    vec3 L0 = vec3(0.02, 0.03, 0.06) * Fex;
    float sunDisk = smoothstep(sunAngularDiameterCos, sunAngularDiameterCos + 0.00002, cosTheta);
    L0 += (sunE * 16000.0 * Fex) * sunDisk;

    // Combine all lighting contributions
    vec3 skyColor = (Lin + L0) * 0.04 + vec3(0.0, 0.00025, 0.00060);

    // Add stars - visible only at night and above horizon
    vec2 starUv = directionToEquirectUV(direction);
    vec3 stars = texture(starTexture, starUv).rgb;
    float nightBlend = smoothstep(-0.25, -0.04, sunDir.y);
    float starMask = smoothstep(-0.05, 0.25, direction.y);
    skyColor += stars * nightBlend * starMask * starIntensity;

    // Apply tone mapping (exposure and gamma correction)
    vec3 mapped = 1.0 - exp(-skyColor * exposure);
    
    // Darken and desaturate horizon line for depth illusion
    float horizonMask = smoothstep(-0.18, 0.04, direction.y);
    mapped = mix(mapped * vec3(0.45, 0.48, 0.55), mapped, horizonMask);

    FragColor = vec4(clamp(mapped, 0.0, 1.0), 1.0);
}
