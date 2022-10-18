#version 330 core
layout(location = 0) in vec3 aPosition;

uniform mat4 lightSpaceMatrix;
uniform mat4 model;

void main() {
	gl_Position = lightSpaceMatrix * model * vec4(aPosition, 1.0f);
}