#version 330 core

layout(location = 0) in vec2 aPosition;
layout(location = 1) in vec2 aTexCoord;

out vec2 fTexCoord;

void main() {
    fTexCoord = aTexCoord;
    gl_Position = vec4(aPosition, 0.0f, 1.0f);
}