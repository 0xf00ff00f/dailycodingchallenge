#version 450 core

layout(location=0) in vec3 position;
layout(location=1) in vec3 normal;
layout(location=2) in vec2 uv;

out vec3 vs_position;
out vec3 vs_normal;
out vec2 vs_uv;

uniform mat4 mvp;
uniform mat4 modelMatrix;

void main(void)
{
    vs_position = vec3(modelMatrix * vec4(position, 1.0));
    vs_normal = normalize(mat3(modelMatrix) * normal); // not quite...
    vs_uv = uv;
    gl_Position = mvp * vec4(position, 1.0);
}
