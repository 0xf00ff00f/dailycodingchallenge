#version 450 core

uniform sampler2DArrayShadow shadowMapTexture;
uniform vec3 eyePosition;
uniform int lightCount;

struct Light
{
    vec4 position;
    mat4 viewProjection;
};

layout (std430, binding=0) buffer Lights
{
    Light lights[];
};

in vec3 vs_normal;
in vec3 vs_position;
in vec4 vs_color;

const vec3 ambient = vec3(0.05);

out vec4 frag_color;

vec3 lightModel()
{
    const mat4 shadowMatrix = mat4(0.5, 0.0, 0.0, 0.0,
                                   0.0, 0.5, 0.0, 0.0,
                                   0.0, 0.0, 0.5, 0.0,
                                   0.5, 0.5, 0.5, 1.0);

    float lightIntensity = 0.0;

    for (int i = 0; i < lightCount; ++i)
    {
        float intensity = (1.0 / float(lightCount)) * max(dot(vs_normal, normalize(lights[i].position.xyz - vs_position)), 0.0);

        // shadow
        vec4 positionInLightSpace = shadowMatrix * lights[i].viewProjection * vec4(vs_position, 1.0);
        vec3 projCoords = positionInLightSpace.xyz / positionInLightSpace.w;
        vec4 textureIndex = vec4(projCoords.xy, float(i), projCoords.z);
        float shadowFactor = texture(shadowMapTexture, textureIndex);

        lightIntensity += shadowFactor * intensity;
    }
    return ambient + lightIntensity * vs_color.xyz;
}

void main(void)
{
    vec3 color = lightModel();
    frag_color = vec4(color, vs_color.w);
}
