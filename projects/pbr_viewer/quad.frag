#version 330 core

out vec4 outColor;

in vec2 fTexCoord;

uniform sampler2D inputTexture;

void main() {
    outColor = texture(inputTexture, fTexCoord);
}