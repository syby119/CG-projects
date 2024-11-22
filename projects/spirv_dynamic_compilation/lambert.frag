#version 430 core

#extension GL_GOOGLE_include_directive : enable

#include "common.glsl"
#define MAX_NUM_LIGHTS 4

layout(location = 0) in vec3 fPosition;
layout(location = 1) in vec3 fNormal;
layout(location = 2) in vec2 fTexCoord;

layout(location = 0) out vec4 outColor;

struct DirectionalLight {
    vec3 direction;
    float intensity;
    vec3 color;
};

layout(std140, binding = 1) uniform UboDirectionalLight {
    uint numDirLights;
    DirectionalLight dirlights[MAX_NUM_LIGHTS];
};

layout(location = 1) uniform vec3 MTL_FIELD_COLOR(albedo);
layout(binding = 0) uniform sampler2D MTL_FIELD(albedoMap);

void main() {
    vec3 normal = normalize(fNormal);

    vec3 color = vec3(0.0);
    for (int i = 0; i < MAX_NUM_LIGHTS; ++i) {
        vec3 lightDir = normalize(-dirlights[i].direction);
        vec3 Li = dirlights[i].intensity * dirlights[i].color;
        vec3 texColor = texture(MTL_FIELD(albedoMap), fTexCoord).rgb;
        vec3 diffuse = Li * max(dot(lightDir, normal), 0.0f) * MTL_FIELD_COLOR(albedo) * texColor;
        color += diffuse;
    }

#if OUTPUT_RED_CHANNAL
    outColor = vec4(color.r, 0.0f, 0.0f, 1.0f);
#else
    outColor = vec4(color, 1.0f);
#endif
}