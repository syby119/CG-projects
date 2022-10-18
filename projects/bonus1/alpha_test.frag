// TODO: modify the code here to achieve alpha testing
#version 330 core
in vec3 fPosition;
in vec3 fNormal;
in vec2 fTexCoord;
out vec4 color;

struct Material {
	vec3 albedo;
	float ka;
	vec3 kd;
	float transparent;
};

struct DirectionalLight {
	vec3 direction;
	float intensity;
	vec3 color;
};

uniform Material material;
uniform DirectionalLight directionalLight;
uniform sampler2D transparentTexture;


void main() {
	vec4 texColor = texture(transparentTexture, fTexCoord);
	vec3 normal = normalize(fNormal);
	vec3 lightDir = normalize(-directionalLight.direction);
	vec3 ambient = material.ka * material.albedo;
	vec3 diffuse = material.kd * texColor.rgb * max(dot(lightDir, normal), 0.0f) * 
					directionalLight.color * directionalLight.intensity;
	color = vec4(ambient + diffuse, 1.0f);
}