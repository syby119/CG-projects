// TODO: modify the following code to support shadow masking
#version 330 core

in vec3 fPosition;
in vec4 fPositionInLightSpace;
in vec3 fNormal;

out vec4 color;

struct Material {
    vec3 ka;
    vec3 kd;
};

struct AmbientLight {
    vec3 color;
    float intensity;
};

struct DirectionalLight {
    vec3 direction;
    float intensity;
    vec3 color;
};

struct PointLight {
    vec3 position;
    float intensity;
    vec3 color;
    float kc;
    float kl;
    float kq;
};

uniform mat4 view;
uniform vec3 viewPosition;

uniform Material material;

uniform AmbientLight ambientLight;
uniform DirectionalLight directionalLight;
uniform PointLight pointLight;
uniform float pointLightZfar;

uniform int directionalFilterRadius;
uniform sampler2D depthTexture;

uniform mat4 lightSpaceMatrices[16];
uniform float cascadeZfars[16];
uniform float cascadeBiasModifiers[16];
uniform int cascadeCount;
uniform sampler2DArray depthTextureArray;

uniform bool enableOmnidirectionalPCF;
uniform samplerCube depthCubeTexture;

vec3 calcAmbientLight() {
    return ambientLight.color * ambientLight.intensity * material.ka;
}

vec3 calcDirectionalLight(vec3 normal) {
    vec3 lightDir = normalize(-directionalLight.direction);
    vec3 diffuse = directionalLight.color * max(dot(lightDir, normal), 0.0f) * material.kd;
    return directionalLight.intensity * diffuse ;
}

vec3 calcPointLight(vec3 normal) {
    vec3 lightDir = normalize(pointLight.position - fPosition);
    vec3 diffuse = pointLight.color * max(dot(lightDir, normal), 0.0f) * material.kd;
    float distance = length(pointLight.position - fPosition);
    float attenuation = 1.0f / (pointLight.kc + pointLight.kl * distance + pointLight.kq * distance * distance);
    return pointLight.intensity * attenuation * diffuse;
}

void main() {
    vec3 normal = normalize(fNormal);
    
    vec3 ambient = calcAmbientLight();
    vec3 diffuse = calcDirectionalLight(normal) + calcPointLight(normal);
    
    color = vec4(ambient + diffuse, 1.0f);
}