#version 430 core

#extension GL_GOOGLE_include_directive : enable
//#extension GL_ARB_bindless_texture : require

#include "common.glsl"

layout(location = 0) in vec3 fPosition;
layout(location = 1) in vec3 fNormal;
layout(location = 2) in vec2 fTexCoord;

layout(location = 0) out vec4 color;

struct DirectionalLight {
    vec3 direction;
    float intensity;
    vec3 color;
};

struct Level1 {
    vec3 c;
};

struct Level0 {
    vec3 d[2][2][2];
    Level1 l1;
    sampler2D ccc;
};

layout(location = 1) uniform vec3 MTL_FIELD(kd);

// name light[2]
//layout(location = 2) uniform DirectionalLight light[2];
layout(location = 2) uniform DirectionalLight light;

layout(location = 6) uniform Level0 composed;

layout(binding = 0) uniform sampler2D MTL_FIELD(tex1);
layout(binding = 1) uniform sampler2D MTL_FIELD(tex2);


void main() {
    vec3 normal = normalize(fNormal);
    vec3 lightDir = normalize(-light.direction);
    vec3 Li = light.intensity * light.color;
    vec3 diffuse = Li * max(dot(lightDir, normal), 0.0f) * MTL_FIELD(kd);

#if OUTPUT_RED_CHANNAL
    color = vec4(diffuse.r, 0.0f, 0.0f, 1.0f);
#else
    color = vec4(diffuse, 1.0f);
#endif
}