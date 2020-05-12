#version 450 core

layout(location=0) in vec3 position;
layout(location=1) in vec3 normal;
layout(location=2) in vec3 color;

uniform mat4 mvp;
uniform mat3 normalMatrix;
uniform mat4 modelMatrix;

out vec3 vs_position;
out vec3 vs_normal;
out vec3 vs_color;

void main(void)
{
    vs_position = vec3(modelMatrix * vec4(position, 1.0));
    vs_normal = normalMatrix * normal;
    vs_color = color;
    gl_Position = mvp * vec4(position, 1.0);
}
