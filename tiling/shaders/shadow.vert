#version 450 core

layout(location=0) in vec2 position;

out vec2 vs_position;
out int vs_instanceID;

void main(void)
{
    vs_position = position;
    vs_instanceID = gl_InstanceID;
}
