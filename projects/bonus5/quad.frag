#version 330 core

out vec4 FragColor;

in vec2 screenTexCoord;

uniform sampler2D frame;

void main() {	
    FragColor = texture(frame, screenTexCoord);	
}