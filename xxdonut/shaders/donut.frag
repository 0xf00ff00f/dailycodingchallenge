#version 450 core

#define PI 3.14159265

in vec3 vs_position;
in vec3 vs_normal;
in vec2 vs_uv;

out vec4 fragColor;

uniform vec3 baseColor;
uniform vec3 lightPosition;
uniform vec3 color;

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
    fragColor = vec4(intensity * color, 1.0);
}
