#version 330 core
out vec4 color;

struct Material {
	vec3 color;
};

uniform Material material;

void main() { 
	color = vec4(material.color, 1.0f);
}