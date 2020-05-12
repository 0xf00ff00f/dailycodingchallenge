#version 450 core

uniform vec3 global_light;
uniform mat4 texture_transform;

in vec3 vs_position;
in vec3 vs_normal;
in vec3 vs_color;

out vec4 frag_color;

const float ambient = 0.15;

void main(void)
{
    float v = ambient + max(dot(vs_normal, normalize(global_light - vs_position)), 0.0);
    frag_color = vec4(v * vs_color, 1.0);
}
