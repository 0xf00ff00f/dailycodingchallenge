#version 450 core

layout(lines) in;
layout(triangle_strip, max_vertices=5) out;

uniform mat4 viewProjectionMatrix;

struct State
{
    mat4 transform;
    float height;
};

layout(std430, binding=0) buffer States
{
    State states[];
};

in vec2 vs_position[];
in int vs_instanceID[];

void main(void)
{
    int tileInstance = vs_instanceID[0];
    float height = states[tileInstance].height;
    mat4 modelMatrix = states[tileInstance].transform;

    mat4 mvp = viewProjectionMatrix * modelMatrix;

    vec2 v0 = 0.99 * vs_position[0];
    vec2 v1 = 0.99 * vs_position[1];

    vec4 p0 = vec4(v0, 0.0, 1.0);
    vec4 p1 = vec4(v1, 0.0, 1.0);
    vec4 p2 = vec4(v0, height, 1.0);
    vec4 p3 = vec4(v1, height, 1.0);
    vec4 p4 = vec4(0.0, 0.0, height, 1.0);

    gl_Position = mvp * p0;
    EmitVertex();

    gl_Position = mvp * p1;
    EmitVertex();

    gl_Position = mvp * p2;
    EmitVertex();

    gl_Position = mvp * p3;
    EmitVertex();

    gl_Position = mvp * p4;
    EmitVertex();
}
