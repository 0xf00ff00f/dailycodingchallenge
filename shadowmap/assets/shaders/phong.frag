#version 450 core

uniform sampler2DShadow shadowMapTexture;
uniform vec3 eyePosition;
uniform vec3 lightPosition;

in vec3 vs_normal;
in vec3 vs_position;
in vec4 vs_color;
in vec4 vs_positionInLightSpace;

const vec3 ambient = vec3(0.25);
const float shininess = 1.0;
const vec3 light_color = vec3(1.0, 1.0, 1.0);
const float kd = 0.5;
const float ks = 0.5;

out vec4 frag_color;

vec3 phong()
{
    vec3 l = normalize(lightPosition - vs_position);

    float diffuse_light = max(dot(vs_normal, l), 0.0);

    vec3 diffuse = kd * diffuse_light * vs_color.xyz;

    vec3 v = normalize(eyePosition - vs_position);
    vec3 h = normalize(l + v);

    float specular_light = pow(max(dot(vs_normal, h), 0.0), shininess);
    if (diffuse_light <= 0.0)
        specular_light = 0.0;

    vec3 specular = ks * specular_light * light_color;

    return diffuse + specular;
}

void main(void)
{
    float shadowFactor = textureProj(shadowMapTexture, vs_positionInLightSpace);
    vec3 color = phong() * shadowFactor;
    frag_color = vec4(color, vs_color.w);
}
