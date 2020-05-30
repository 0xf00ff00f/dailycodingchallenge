#version 450 core

layout(lines) in;
layout(triangle_strip, max_vertices=14) out;

uniform mat4 viewProjectionMatrix;
uniform mat4 modelMatrix;
uniform mat4 lightViewProjection;

in vec2 vs_position[];

out vec3 gs_position;
out vec3 gs_normal;
out vec3 gs_color;
out vec4 gs_positionInLightSpace;

void emit_vertex(vec4 pos, vec3 normal, vec3 color)
{
     const mat4 shadowMatrix = mat4(0.5, 0.0, 0.0, 0.0,
                                    0.0, 0.5, 0.0, 0.0,
                                    0.0, 0.0, 0.5, 0.0,
                                    0.5, 0.5, 0.5, 1.0);

    mat4 mvp = viewProjectionMatrix * modelMatrix;
    gs_position = vec3(modelMatrix * pos);
    gs_positionInLightSpace = shadowMatrix * lightViewProjection * modelMatrix * pos;
    gs_normal = normal;
    gs_color = color;
    gl_Position = mvp * pos;
    EmitVertex();
}

void main(void)
{
    mat3 normalMatrix = mat3(modelMatrix);
    mat4 mvp = viewProjectionMatrix * modelMatrix;

    vec2 d = vs_position[0] - vs_position[1];
    vec3 side_normal = normalMatrix * normalize(vec3(-d.y, d.x, 0.0));
    vec3 up_normal = normalMatrix * vec3(0.0, 0.0, 1.0);

    vec2 v0 = vs_position[0];
    vec2 v1 = vs_position[1];

    const float height = 0.2;
    const vec3 front_color = vec3(1, 1, 0);
    const vec3 back_color = vec3(0.46, 0.35, .6);

    vec4 p00 = vec4(0.0, 0.0, height, 1.0);

    vec4 p10 = vec4(v0, height, 1.0);
    vec4 p20 = vec4(v1, height, 1.0);

    vec4 p11 = vec4(v0, 0.0, 1.0);
    vec4 p21 = vec4(v1, 0.0, 1.0);

    vec4 p12 = vec4(v0, -height, 1.0);
    vec4 p22 = vec4(v1, -height, 1.0);

    vec4 p02 = vec4(0.0, 0.0, -height, 1.0);

    emit_vertex(p00, up_normal, front_color);
    emit_vertex(p10, up_normal, front_color);
    emit_vertex(p20, up_normal, front_color);
    emit_vertex(p10, side_normal, front_color);
    emit_vertex(p20, side_normal, front_color);
    emit_vertex(p11, side_normal, front_color);
    emit_vertex(p21, side_normal, front_color);
    emit_vertex(p11, side_normal, back_color);
    emit_vertex(p21, side_normal, back_color);
    emit_vertex(p12, side_normal, back_color);
    emit_vertex(p22, side_normal, back_color);
    emit_vertex(p12, -up_normal, back_color);
    emit_vertex(p22, -up_normal, back_color);
    emit_vertex(p02, -up_normal, back_color);
}
