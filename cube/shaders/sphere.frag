#version 450 core

uniform vec3 global_light;
uniform vec3 pattern_dir;
uniform float pattern_len;
uniform mat4 texture_transform;

in vec3 vs_position;
in vec3 vs_normal;

out vec4 frag_color;

const float ambient = 0.15;

float random(vec3 st)
{
    return fract(sin(dot(st, vec3(12.9898, 78.233, 55.333))) * 43758.5453123);
}

vec4 pattern(vec3 id, vec3 p, vec3 offs)
{
    id += offs;
    float r = 0.7 * random(id);
    vec3 o = vec3(random(id + vec3(0, 1, 0)), random(id + vec3(1, 0, 0)), random(id + vec3(0, 0, 1))) + offs;
    float d = distance(p, o);
    vec4 color = vec4(random(id), random(id + vec3(1, 1, 0)), random(id + vec3(1, 1, 1)), 1);
    float a = clamp(1 - smoothstep(r - .05, r, d), 0, 1);
    return a * color;
}

void main(void)
{
    vec3 uv = (texture_transform * vec4(vs_position, 1)).xyz * 2.0;

    vec3 p = fract(uv) - 0.5;
    vec3 id = floor(uv);

    vec4 color = vec4(0.1);
    for (float i = -1; i <= 1; ++i)
    {
        for (float j = -1; j <= 1; ++j)
        {
            for (float k = -1; k <= 1; ++k)
            {
                color += pattern(id, p, vec3(i, j, k));
            }
        }
    }
    color.w = clamp(color.w, 0.0, 0.8);
    color = clamp(color, 0, 1);

    float v = ambient + max(dot(vs_normal, normalize(global_light - vs_position)), 0.0);
    frag_color = v * color;
}
