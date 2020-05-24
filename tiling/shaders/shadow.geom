#version 450 core

layout(lines) in;
layout(triangle_strip, max_vertices=5) out;

uniform mat4 viewProjectionMatrix;
uniform mat4 modelMatrix;
uniform float height;

in vec2 vs_position[];

void main(void)
{
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
