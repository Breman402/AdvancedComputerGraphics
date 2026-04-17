#version 420 core

layout(location = 0) in vec3 position;

uniform mat4 mvpMatrix;

uniform mat4 modelMatrix;

out vec3 vWorldPos;
out vec3 vWorldNormal;

void main()
{
    // Transform vertex position to world space
    vec4 worldPos = modelMatrix * vec4(position, 1.0);
    vWorldPos = worldPos.xyz;
    
    // Transform and normalize vertex normal to world space
    vWorldNormal = normalize(mat3(modelMatrix) * position);
    
    // Transform vertex position to clip space for rasterization
    gl_Position = mvpMatrix * vec4(position, 1.0);
}