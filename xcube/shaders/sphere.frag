#version 450 core

in vec3 vs_normal;
in vec3 vs_position;
in vec4 vs_color;

uniform vec3 eyePosition;

const vec3 light_position = vec3(2.0, -3.0, 1.0);
const vec3 ambient = vec3(0.15);
const float shininess = 8.0;
const vec3 light_color = vec3(1.0, 1.0, 1.0);
const float kd = 0.5;
const float ks = 0.5;

out vec4 frag_color;

void main(void)
{
    vec3 l = normalize(light_position - vs_position);
    float diffuse_light = max(dot(vs_normal, l), 0.0);
    vec3 diffuse = kd * diffuse_light * vs_color.xyz;
    vec3 v = normalize(eyePosition - vs_position);
    vec3 h = normalize(l + v);
    float specular_light = pow(max(dot(vs_normal, h), 0.0), shininess);
    if (diffuse_light <= 0.0)
        specular_light = 0.0;
    vec3 specular = ks * specular_light * light_color;
    frag_color = vec4(ambient + diffuse + specular, vs_color.w);
}
