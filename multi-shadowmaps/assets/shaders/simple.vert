#version 450 core

layout(location=0) in vec3 position;
layout(location=1) in vec3 normal;
layout(location=2) in vec2 uv;

uniform mat4 modelMatrix;
uniform mat4 viewMatrix;
uniform mat4 projectionMatrix;

out vec3 vs_position;
out vec3 vs_normal;
out vec4 vs_color;

void main(void)
{
    vs_position = vec3(modelMatrix * vec4(position, 1.0));
    vs_normal = normalize(mat3(modelMatrix) * normal); // not quite correct
    vs_color = vec4(1.0);
    gl_Position = projectionMatrix * viewMatrix * modelMatrix * vec4(position, 1.0);
}
