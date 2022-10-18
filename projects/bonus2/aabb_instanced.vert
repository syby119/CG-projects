#version 330 core
layout(location = 0) in vec3 aPosition;
layout(location = 1) in mat4 aInstanceMatrix;

uniform mat4 projection;
uniform mat4 view;

void main() {
    gl_Position = projection * view * aInstanceMatrix * vec4(aPosition, 1.0f);
}