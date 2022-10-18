#version 330 core
in vec3 fPosition;
in vec3 fNormal;
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

vec3 lambertShading() {
	vec3 normal = normalize(fNormal);
	if (!gl_FrontFacing) { 
		normal = -normal;
	}
	vec3 lightDir = normalize(-directionalLight.direction);
	vec3 ambient = material.ka * material.albedo ;
	vec3 diffuse = material.kd * max(dot(lightDir, normal), 0.0f) * 
				   directionalLight.color * directionalLight.intensity;
	return ambient + diffuse;
}

void main() {
	vec3 premultiedColor = lambertShading() * material.transparent;
	color = vec4(premultiedColor, 1.0f - material.transparent);
}