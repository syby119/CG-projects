#version 330 core
layout(location = 0) in vec2 aPosition;
layout(location = 1) in vec2 aTexCoords;

out vec2 fTexCoords;

void main() {
	fTexCoords = aTexCoords;
	gl_Position = vec4(aPosition, 0.0f, 1.0f);
}