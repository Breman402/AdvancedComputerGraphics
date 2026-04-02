#version 410 core

layout(location = 0) in vec3 position;

uniform mat4 mvpMatrix;

out vec3 vDir;

void main()
{
    vDir = normalize(position);
    gl_Position = mvpMatrix * vec4(position, 1.0);
}