#version 450 core

uniform vec3 lightPosition;
uniform vec3 color;

in vec3 gs_position;
in vec3 gs_normal;

out vec4 fragColor;

void main(void)
{
    float intensity = 0.15 + max(dot(gs_normal, normalize(lightPosition - gs_position)), 0.0);
    fragColor = vec4(intensity * vec3(1.0), 1.0); // vec4(gs_normal, 1.0); // vec4(intensity * color, 1.0);
}
