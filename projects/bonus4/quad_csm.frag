#version 330 core
out vec4 color;
in vec2 fTexCoord;

uniform int level;
uniform sampler2DArray depthTextureArray;

void main() {
	float depth = texture(depthTextureArray, vec3(fTexCoord, level)).r;
	color = vec4(vec3(pow(depth, 3.0f)), 1.0f);
}