#version 450 core

#define PI 3.14159265

uniform sampler2DShadow shadowMapTexture;
uniform vec3 baseColor;
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

    const float range = 1;
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
    return min(factor + 0.75, 1.0);
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
    vec2 p = vs_uv;
    p.x *= 4.0;
    p = fract(p);

    const vec3 strip_colors[] = vec3[](vec3(1, 0, 0), vec3(0, 1, 0), vec3(0.5, 0, 1), vec3(1, 1, 0));
    vec3 strip_color = strip_colors[int(mod(vs_uv.x * 4, 4))];

    float m = 0.5 + 0.125 * sin(p.y * 8.0 * PI);

    const float e0 = 0.1;
    const float e1 = 0.15;

    float t = smoothstep(m - e0, m - e0, p.x) * (1.0 - smoothstep(m + e0, m + e1, p.x));
    vec3 color = mix(baseColor, strip_color, t);

    // float intensity = max(dot(vs_normal, normalize(lightPosition - vs_position)), 0.0);
    float intensity = mix(abs(dot(vs_normal, normalize(lightPosition - vs_position))), 1, t);
    intensity *= shadowFactor();
    fragColor = vec4(intensity * color, 1.0);
}
