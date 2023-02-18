#version 330 core
layout(location = 0) in vec3 aPosition;

out vec4 fPosition;

uniform mat4 model;
uniform mat4 lightSpaceMatrix;

void main() {
    fPosition = model * vec4(aPosition, 1.0);
    gl_Position = lightSpaceMatrix * fPosition;
}