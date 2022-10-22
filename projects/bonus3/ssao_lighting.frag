#version 330 core
layout(location = 0) out vec4 fragColor;

in vec2 screenTexCoord;

struct PointLight {
    vec3 position;
    vec3 color;
    float intensity;
    float kc;
    float kl;
    float kq;
};

uniform PointLight light;

uniform sampler2D gPosition;
uniform sampler2D gNormal;
uniform sampler2D gAlbedo;
uniform sampler2D ssaoResult;

void main() {
    vec3 position = texture(gPosition, screenTexCoord).xyz;
    vec3 normal = texture(gNormal, screenTexCoord).xyz;
    vec3 albedo = texture(gAlbedo, screenTexCoord).rgb;
    float occlusion = texture(ssaoResult, screenTexCoord).x;

    //TODO: perform lambert shading
    fragColor = vec4(normal, 1.0f);
}