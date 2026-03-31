#version 420

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 texcoord;

uniform mat4 lightViewProj;
uniform mat4 modelMatrix;

uniform sampler2D earthHeightTex;
uniform float baseRadius;
uniform float heightScale;

void main()
{
    // Same displacement as earth.vert
    vec3 dir = normalize(position);
    float h = texture(earthHeightTex, texcoord).r;
    float r = baseRadius + h * heightScale;
    vec3 displacedLocal = dir * r;

    gl_Position = lightViewProj * modelMatrix * vec4(displacedLocal, 1.0);
}
