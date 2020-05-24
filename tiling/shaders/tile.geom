#version 450 core

layout(lines) in;
layout(triangle_strip, max_vertices=7) out;

uniform mat4 viewProjectionMatrix;
uniform mat4 lightViewProjection;

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

out vec3 gs_position;
out vec3 gs_normal;
out vec4 gs_positionInLightSpace;

void main(void)
{
    int tileInstance = vs_instanceID[0];
    float height = states[tileInstance].height;
    mat4 modelMatrix = states[tileInstance].transform;

    const mat4 shadowMatrix = mat4(0.5, 0.0, 0.0, 0.0,
                                   0.0, 0.5, 0.0, 0.0,
                                   0.0, 0.0, 0.5, 0.0,
                                   0.5, 0.5, 0.5, 1.0);

    mat3 normalMatrix = mat3(modelMatrix);
    mat4 mvp = viewProjectionMatrix * modelMatrix;

    vec2 d = vs_position[0] - vs_position[1];
    vec3 side_normal = normalMatrix * normalize(vec3(-d.y, d.x, 0.0));
    vec3 up_normal = normalMatrix * vec3(0.0, 0.0, 1.0);

    vec2 v0 = 0.99 * vs_position[0];
    vec2 v1 = 0.99 * vs_position[1];

    vec4 p0 = vec4(v0, 0.0, 1.0);
    vec4 p1 = vec4(v1, 0.0, 1.0);
    vec4 p2 = vec4(v0, height, 1.0);
    vec4 p3 = vec4(v1, height, 1.0);
    vec4 p4 = vec4(0.0, 0.0, height, 1.0);

    gs_position = vec3(modelMatrix * p0);
    gs_positionInLightSpace = shadowMatrix * lightViewProjection * modelMatrix * p0;
    gs_normal = side_normal;
    gl_Position = mvp * p0;
    EmitVertex();

    gs_position = vec3(modelMatrix * p1);
    gs_positionInLightSpace = shadowMatrix * lightViewProjection * modelMatrix * p1;
    gs_normal = side_normal;
    gl_Position = mvp * p1;
    EmitVertex();

    gs_position = vec3(modelMatrix * p2);
    gs_positionInLightSpace = shadowMatrix * lightViewProjection * modelMatrix * p2;
    gs_normal = side_normal;
    gl_Position = mvp * p2;
    EmitVertex();

    gs_position = vec3(modelMatrix * p3);
    gs_positionInLightSpace = shadowMatrix * lightViewProjection * modelMatrix * p3;
    gs_normal = side_normal;
    gl_Position = mvp * p3;
    EmitVertex();

    gs_position = vec3(modelMatrix * p2);
    gs_positionInLightSpace = shadowMatrix * lightViewProjection * modelMatrix * p2;
    gs_normal = up_normal;
    gl_Position = mvp * p2;
    EmitVertex();

    gs_position = vec3(modelMatrix * p3);
    gs_positionInLightSpace = shadowMatrix * lightViewProjection * modelMatrix * p3;
    gs_normal = up_normal;
    gl_Position = mvp * p3;
    EmitVertex();

    gs_position = vec3(modelMatrix * p4);
    gs_positionInLightSpace = shadowMatrix * lightViewProjection * modelMatrix * p4;
    gs_normal = up_normal;
    gl_Position = mvp * p4;
    EmitVertex();
}
