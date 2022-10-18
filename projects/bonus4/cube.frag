#version 330 core
out vec4 color;
in vec3 fPosition;

uniform samplerCube depthCubeTexture;

void main() {
	color = vec4(texture(depthCubeTexture, fPosition).rrr, 1.0f);
}