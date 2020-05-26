#version 450 core

in vec3 vs_position;
in vec3 vs_normal;
in vec2 vs_uv;

out vec4 fragColor;

uniform vec3 lightPosition;
uniform vec3 color;

float random(vec2 st)
{
    return fract(sin(dot(st.xy, vec2(12.9898, 78.233))) * 43758.5453123);
}

float pattern(vec2 id, vec2 p, vec2 offs)
{
    id += offs;
    float r = 0.5 * random(id);
    vec2 o = vec2(random(id + vec2(0, 1)), random(id + vec2(1, 0))) + offs;
    float d = distance(p, o);
    return 1.0 - smoothstep(r - .05, r, d);
}

void main(void)
{
    vec2 uv = vec2(vs_uv.x * 8.0, vs_uv.y);
    uv *= 8.0;

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
    l = clamp(l, 0, 1);
    vec3 color = vec3(l);

    // float intensity = max(dot(vs_normal, normalize(lightPosition - vs_position)), 0.0);
    float intensity = abs(dot(vs_normal, normalize(lightPosition - vs_position)));
    fragColor = vec4(intensity * color, 1.0);
}
