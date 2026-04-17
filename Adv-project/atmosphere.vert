#version 410 core

layout(location = 0) in vec3 position;

uniform mat4 mvpMatrix;

out vec3 vViewDir;

void main()
{
    vViewDir = normalize(position);
    gl_Position = mvpMatrix * vec4(position, 1.0);
}
