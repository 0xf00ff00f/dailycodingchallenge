#version 450 core

layout(location=0) in vec2 position;

out vec2 vs_position;

void main(void)
{
    vs_position = position;
}
