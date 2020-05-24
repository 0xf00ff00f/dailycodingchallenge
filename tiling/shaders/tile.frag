#version 450 core

uniform sampler2DShadow shadowMapTexture;
uniform vec3 lightPosition;
uniform vec3 color;

in vec3 gs_position;
in vec3 gs_normal;
in vec4 gs_positionInLightSpace;

out vec4 fragColor;

void main(void)
{
    float shadowFactor = textureProj(shadowMapTexture, gs_positionInLightSpace);
    shadowFactor = min(shadowFactor + 0.25, 1.0);
    float intensity = 0.15 + max(dot(gs_normal, normalize(lightPosition - gs_position)), 0.0);
    fragColor = vec4(shadowFactor * intensity * vec3(1.0), 1.0); // vec4(gs_normal, 1.0); // vec4(intensity * color, 1.0);
}
