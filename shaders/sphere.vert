#version 450 core

layout(location=0) in vec3 position;
layout(location=1) in vec3 normal;
layout(location=2) in vec2 uv;

layout(location=0) uniform mat4 mvp;
layout(location=1) uniform mat3 normalMatrix;
layout(location=2) uniform mat4 modelMatrix;

out vec3 vs_position;
out vec3 vs_normal;
out vec2 vs_uv;

void main(void)
{
    vs_position = vec3(modelMatrix * vec4(position, 1.0));
    vs_normal = normalMatrix * normal;
    vs_uv = uv;
    gl_Position = mvp*vec4(position, 1.0);
}
