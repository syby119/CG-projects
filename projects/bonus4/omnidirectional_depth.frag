#version 330 core
in vec4 fPosition;

uniform vec3 lightPosition;
uniform float zFar;

void main() {
	gl_FragDepth = length(fPosition.xyz - lightPosition) / zFar;
}