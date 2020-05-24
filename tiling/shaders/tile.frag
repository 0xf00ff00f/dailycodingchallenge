#version 450 core

uniform sampler2DShadow shadowMapTexture;
uniform vec3 lightPosition;
uniform vec3 color;

in vec3 gs_position;
in vec3 gs_normal;
in vec4 gs_positionInLightSpace;

out vec4 fragColor;

float shadowFactor()
{
#if 0
    float factor = textureProj(shadowMapTexture, gs_positionInLightSpace);
#else
    vec3 projCoords = gs_positionInLightSpace.xyz / gs_positionInLightSpace.w;
    vec2 uvCoords = projCoords.xy;

    ivec2 texDim = textureSize(shadowMapTexture, 0).xy;
    float xOffset = 1.0 / float(texDim.x);
    float yOffset = 1.0 / float(texDim.y);

    const float range = 3;

    float factor = 0.0;
    for (float y = -range; y <= range; ++y)
    {
        for (float x = -range; x <= range; ++x)
        {
            factor += texture(shadowMapTexture, vec3(projCoords + vec3(x * xOffset, y * yOffset, 0)));
        }
    }
    factor /= ((range * 2 + 1) * (range * 2 + 1));
#endif
    return min(factor + 0.5, 1.0);
}

void main(void)
{
    float intensity = 0.15 + max(dot(gs_normal, normalize(lightPosition - gs_position)), 0.0);
    intensity *= shadowFactor();
    fragColor = vec4(intensity * vec3(1.0), 1.0); // vec4(gs_normal, 1.0); // vec4(intensity * color, 1.0);
}
