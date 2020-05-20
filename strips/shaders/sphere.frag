#version 450 core

in vec3 vs_position;
in vec3 vs_normal;
in vec2 vs_uv;
in vec4 vs_positionInLightSpace;

out vec4 fragColor;

uniform sampler2DShadow shadowMapTexture;
uniform vec3 lightPosition;
uniform vec3 color;
uniform vec2 vRange;

float shadowFactor()
{
#if 0
    float factor = textureProj(shadowMapTexture, vs_positionInLightSpace);
#else
    vec3 projCoords = vs_positionInLightSpace.xyz / vs_positionInLightSpace.w;
    vec2 uvCoords = projCoords.xy;

    ivec2 texDim = textureSize(shadowMapTexture, 0).xy;
    float xOffset = 1.0 / float(texDim.x);
    float yOffset = 1.0 / float(texDim.y);

    const float range = 5;

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
    return min(factor + 0.75, 1.0);
}

void main(void)
{
    float vStart = vRange.x;
    float vEnd = vRange.y;
    float alpha = 0.0;
    const float border = 0.01;
    if (vEnd > vStart)
    {
        if (vs_uv.y < vStart || vs_uv.y > vEnd)
            discard;
        alpha = smoothstep(vStart, vStart + border, vs_uv.y) * (1.0 - smoothstep(vEnd - border, vEnd, vs_uv.y));
    }
    else
    {
        if (vs_uv.y > vEnd && vs_uv.y < vStart)
            discard;
        alpha = smoothstep(vStart, vStart + border, vs_uv.y) + (1.0 - smoothstep(vEnd - border, vEnd, vs_uv.y));
    }

    float intensity = max(dot(vs_normal, normalize(lightPosition - vs_position)), 0.0);

    float u = fract(128.0 * (vs_uv.y + vStart) + vs_uv.x);
    if (u > 0.8 || vs_uv.x < .1 || vs_uv.x > .9)
        intensity *= 0.75;

    // intensity *= shadowFactor();

    fragColor = vec4(intensity * color, alpha);
}
