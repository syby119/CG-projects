#version 330 core

layout(location = 0) in vec3 aPosition;

layout(shared) uniform uboCamera {
	mat4 projection;
	mat4 view;
	vec3 viewPosition;
};

out vec3 fWorldPos;

void main() {
	fWorldPos = aPosition;
	gl_Position = (projection * mat4(mat3(view)) * vec4(fWorldPos, 1.0f)).xyww;
}