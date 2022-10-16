#version 330 core

layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoord0;
layout(location = 3) in vec2 aTexCoord1;

layout(shared) uniform uboCamera {
	mat4 projection;
	mat4 view;
	vec3 viewPosition;
};

out vec3 fWorldPos;
out vec3 fNormal;
out vec2 fTexCoord0;
out vec2 fTexCoord1;

uniform mat4 model;

void main() {
	fWorldPos = vec3(model * vec4(aPosition, 1.0f));
	fNormal = mat3(transpose(inverse(model))) * aNormal;
	fTexCoord0 = aTexCoord0;
	fTexCoord1 = aTexCoord1;
	
	gl_Position = projection * view * vec4(fWorldPos, 1.0f);
}