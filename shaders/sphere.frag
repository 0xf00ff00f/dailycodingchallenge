#version 450 core

uniform vec3 global_light;
uniform vec3 pattern_dir;
uniform float pattern_len;
uniform vec2 uv_offset;

in vec3 vs_position;
in vec3 vs_normal;
in vec2 vs_uv;

out vec4 frag_color;

const float ambient = 0.15;

float random(vec2 st)
{
    return fract(sin(dot(st.xy, vec2(12.9898, 78.233))) * 43758.5453123);
}

float pattern(vec2 id, vec2 p, vec2 offs)
{
    id = mod(id + offs, 16);
    float r = 0.5 * random(id);
    vec2 o = vec2(random(id + vec2(0, 1)), random(id + vec2(1, 0))) + offs;
    float d = distance(p, o);
    return 1.0 - smoothstep(r - .05, r, d);
}

void main(void)
{
    vec2 uv = vec2(vs_uv.x * 4.0, vs_uv.y);
    uv += uv_offset;
    uv *= 16.0;

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

    float v = ambient + max(dot(vs_normal, normalize(global_light - vs_position)), 0.0);
    frag_color = vec4(v * color, 0.4);
}
