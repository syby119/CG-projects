#version 330 core
in vec3 fPosition;
in vec3 fNormal;
in vec2 fTexCoord;
out vec4 color;

struct Material {
    vec3 kd;
};

struct DirectionalLight {
    vec3 direction;
    float intensity;
    vec3 color;
};

uniform Material material;
uniform sampler2D mapKd;
uniform DirectionalLight light;

void main() {
    vec3 normal = normalize(fNormal);
    vec3 lightDir = normalize(-light.direction);
    vec3 diffuse = light.intensity * light.color * max(dot(lightDir, normal), 0.0f) * 
                   material.kd * texture(mapKd, fTexCoord).rgb;
    color = vec4(diffuse, 1.0f);
}