#version 330 core
layout(location = 0) in vec3 aPosition;

out vec3 fPosition;

uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;

void main() {
	fPosition = (model * vec4(aPosition, 1.0f)).xyz;
	gl_Position = projection * mat4(mat3(view)) * vec4(fPosition, 1.0f);
}