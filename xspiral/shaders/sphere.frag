#version 450 core

in vec3 vs_position;
in vec3 vs_normal;
in vec2 vs_uv;

out vec4 fragColor;

uniform vec3 lightPosition;
uniform vec3 color;
uniform vec2 vRange;

void main(void)
{
    float vStart = vRange.x;
    float vEnd = vRange.y;
    float alpha = 0.0;
    const float border = 0.001;
    if (vStart.x != -1)
    {
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
    }
    else
    {
        alpha = 1.0;
    }

    // float intensity = max(dot(vs_normal, normalize(lightPosition - vs_position)), 0.0);
    float intensity = abs(dot(vs_normal, normalize(lightPosition - vs_position)));

    float u = fract(128.0 * (vs_uv.y + vStart) + vs_uv.x);
    if (/* u > 0.8 || */ vs_uv.x < .1 || vs_uv.x > .9)
        intensity *= 0.75;

    fragColor = vec4(intensity * color, alpha);
}
