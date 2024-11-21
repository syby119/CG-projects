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
    vec3 d[2][3];
    Level1 l1;
    sampler2D ccc;
};

layout(location = 1) uniform vec3 MTL_FIELD(kd);

layout(location = 2) uniform DirectionalLight light;

//layout(location = 6) uniform Level0 composed;
layout(location = 6) uniform Level1 composed[2];
//layout(location = 6) uniform float array[2];

//layout(binding = 0) uniform sampler2D MTL_FIELD(tex1);
//layout(binding = 1) uniform sampler2D MTL_FIELD(tex2);

layout(binding = 6) uniform sampler2D texs[2];

layout(binding = 1) buffer SSBO {
    float ssbo[];
};

layout(rg32ui, binding = 0)
uniform readonly uimage2D pointLightGrid;

layout(binding = 0, offset = 12) uniform atomic_uint one;
layout(binding = 0) uniform atomic_uint two;
layout(binding = 0, offset = 4) uniform atomic_uint three;

//layout(binding = 1) uniform atomic_uint four;
//layout(binding = 1) uniform atomic_uint five;
//layout(binding = 1, offset = 20) uniform atomic_uint six;
//layout(binding = 0) uniform atomic_uint seven;

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