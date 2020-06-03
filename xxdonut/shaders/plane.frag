#version 450 core

uniform sampler2DShadow shadowMapTexture;
uniform vec3 lightPosition;
uniform vec3 color;

in vec3 vs_position;
in vec3 vs_normal;
in vec2 vs_uv;
in vec4 vs_positionInLightSpace;

out vec4 fragColor;

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
    const float shadowFactor = 0.25;

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

float random(vec2 st)
{
    return fract(sin(dot(st.xy, vec2(12.9898, 78.233))) * 43758.5453123);
}

float pattern(vec2 id, vec2 p, vec2 offs)
{
    id = mod(id + offs, 32);
    float r = 0.5 * random(id);
    vec2 o = vec2(random(id + vec2(0, 1)), random(id + vec2(1, 0))) + offs;
    float d = distance(p, o);
    return 1.0 - smoothstep(r - .05, r, d);
}

void main(void)
{
    /*
    vec2 uv = vs_uv * 32.0;

    vec2 p = fract(uv) - 0.5;
    vec2 id = floor(uv);

    float l = 0;
    for (float i = -1; i <= 1; ++i)
    {
        for (float j = -1; j <= 1; ++j)
        {
            l += pattern(id, p, vec2(i, j));
        }
    }
    vec3 color = mix(vec3(0, 0, 1), vec3(1), clamp(l, 0, 1));
    */
    vec3 color = vec3(1);

    // float intensity = max(dot(vs_normal, normalize(lightPosition - vs_position)), 0.0);
    float intensity = min(0.25 + abs(dot(vs_normal, normalize(lightPosition - vs_position))), 1.0);
    intensity *= shadowFactor();
    fragColor = vec4(intensity * color, 0.75);
}
