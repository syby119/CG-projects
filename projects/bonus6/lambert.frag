#version 430 core

struct Material {
    vec3 kd;
};

struct DirectionalLight {
    vec3 direction;
    float intensity;
    vec3 color;
};

uniform Material material;
uniform DirectionalLight directionalLight;

in PerVertexData {
    vec3 positionWS;
    vec3 normalWS;
    vec2 texCoord;
} fragIn;

layout(location = 0) out vec4 fragColor;

vec3 calcDirectionalLight(vec3 normal) {
    vec3 lightDir = normalize(-directionalLight.direction);
    vec3 diffuse = directionalLight.color * max(dot(lightDir, normal), 0.0f) * material.kd;
    return directionalLight.intensity * diffuse ;
}

void main() {
    vec3 normal = normalize(fragIn.normalWS);
    vec3 diffuse = calcDirectionalLight(normal);
    fragColor = vec4(diffuse, 1.0f);
};