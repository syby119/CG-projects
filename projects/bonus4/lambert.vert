#version 330 core
layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoord;

out vec3 fPosition;
out vec4 fPositionInLightSpace;
out vec3 fNormal;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform mat4 lightSpaceMatrix;

void main() {
	fPosition = vec3(model * vec4(aPosition, 1.0f));
	fPositionInLightSpace = lightSpaceMatrix * vec4(fPosition, 1.0f);
	fNormal = mat3(transpose(inverse(model))) * aNormal;
	gl_Position = projection * view * model * vec4(aPosition, 1.0f);
}