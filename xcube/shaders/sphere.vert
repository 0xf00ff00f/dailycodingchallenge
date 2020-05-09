#version 450 core

layout(location=0) in vec3 position;
layout(location=1) in vec3 normal;
layout(location=2) in vec2 uv;

struct State
{
    mat4 transform;
    vec4 color;
};

layout(std430, binding=0) buffer Transforms
{
    State states[];
};

uniform mat4 modelMatrix;
uniform mat4 viewMatrix;
uniform mat4 projectionMatrix;

out vec3 vs_position;
out vec3 vs_normal;
out vec4 vs_color;

void main(void)
{
    mat4 instanceModelMatrix = modelMatrix * states[gl_InstanceID].transform;
    vs_position = vec3(instanceModelMatrix * vec4(position, 1.0));
    vs_normal = normalize(mat3(instanceModelMatrix) * normal); // not quite correct
    vs_color = states[gl_InstanceID].color;
    gl_Position = projectionMatrix * viewMatrix * instanceModelMatrix * vec4(position, 1.0);
}
