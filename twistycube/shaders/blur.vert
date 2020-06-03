#version 450 core

layout(location=0) in vec2 position;
layout(location=1) in vec2 vertex_tex_coords;

out vec2 tex_coords;

void main(void)
{
    gl_Position = vec4(position, 0.0, 1.0);
    tex_coords = vertex_tex_coords;
}
