#version 450 core

layout(lines) in;
layout(triangle_strip, max_vertices=6) out;

uniform mat4 viewProjectionMatrix;
uniform mat4 modelMatrix;

in vec2 vs_position[];

out vec3 gs_position;

void emit_vertex(vec4 pos)
{
    mat4 mvp = viewProjectionMatrix * modelMatrix;
    gl_Position = mvp * pos;
    EmitVertex();
}

void main(void)
{
    mat3 normalMatrix = mat3(modelMatrix);
    mat4 mvp = viewProjectionMatrix * modelMatrix;

    vec2 v0 = vs_position[0];
    vec2 v1 = vs_position[1];

    const float height = 0.2;

    vec4 p00 = vec4(0.0, 0.0, height, 1.0);

    vec4 p10 = vec4(v0, height, 1.0);
    vec4 p20 = vec4(v1, height, 1.0);

    vec4 p12 = vec4(v0, -height, 1.0);
    vec4 p22 = vec4(v1, -height, 1.0);

    vec4 p02 = vec4(0.0, 0.0, -height, 1.0);

    emit_vertex(p00);
    emit_vertex(p10);
    emit_vertex(p20);
    emit_vertex(p12);
    emit_vertex(p22);
    emit_vertex(p02);
}
