#version 410 core

out vec4 FragColor;

in vec3 vDir;

uniform vec3 sunColor;
uniform sampler2D skyDayTimeTexture;
uniform sampler2D skyNightTimeTexture;
uniform sampler2D skyDawnTexture;
uniform sampler2D skyDuskTexture;
uniform bool hasSkyTextures;
uniform float sunTimeOfDayAngle;

#define PI 3.14159265359
#define TAU 6.28318530718

vec2 directionToEquirectUV(vec3 d)
{
    d = normalize(d);

    float u = atan(d.z, d.x) / TAU + 0.5;
    float v = asin(clamp(d.y, -1.0, 1.0)) / PI + 0.5;
    
    float eps = 0.5 / 2048.0; // if texture width is 2048
    u = fract(u);
    u = mix(eps, 1.0 - eps, u);

    return vec2(u, v);
}

void main()
{
    if (!hasSkyTextures)
    {
        FragColor = vec4(sunColor, 1.0);
        return;
    }

    vec2 uv = directionToEquirectUV(vDir);

    vec3 skyNight = texture(skyNightTimeTexture, uv).rgb;
    vec3 skyDawn  = texture(skyDawnTexture, uv).rgb;
    vec3 skyDay   = texture(skyDayTimeTexture, uv).rgb;
    vec3 skyDusk  = texture(skyDuskTexture, uv).rgb;

    float a = mod(sunTimeOfDayAngle, TAU);
    if (a < 0.0)
    {
        a += TAU;
    }

    vec3 skyColor;

    // sunrise -> noon
    if (a < 0.5 * PI)
    {
        float t = smoothstep(0.0, 0.5 * PI, a);
        skyColor = mix(skyDawn, skyDay, t);
    }
    // noon -> sunset
    else if (a < PI)
    {
        float t = smoothstep(0.5 * PI, PI, a);
        skyColor = mix(skyDay, skyDusk, t);
    }
    // sunset -> midnight
    else if (a < 1.5 * PI)
    {
        float t = smoothstep(PI, 1.5 * PI, a);
        skyColor = mix(skyDusk, skyNight, t);
    }
    // midnight -> sunrise
    else
    {
        float t = smoothstep(1.5 * PI, TAU, a);
        skyColor = mix(skyNight, skyDawn, t);
    }

    FragColor = vec4(skyColor, 1.0);
}