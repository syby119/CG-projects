#version 330 core
out vec4 color;
in vec2 fTexCoord;

uniform sampler2D depthTexture;

void main() {
	float depth = texture(depthTexture, fTexCoord).r;
	color = vec4(vec3(depth), 1.0f);
}