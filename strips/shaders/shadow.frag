#version 450 core

in vec2 vs_uv;

uniform vec2 vRange;

void main()
{
    float vStart = vRange.x;
    float vEnd = vRange.y;
    if (vEnd > vStart)
    {
        if (vs_uv.y < vStart || vs_uv.y > vEnd)
            discard;
    }
    else
    {
        if (vs_uv.y > vEnd && vs_uv.y < vStart)
            discard;
    }
}
