#version 330 core

layout(location = 0) in vec3 aPosition;

out vec3 fWorldPos;

uniform mat4 projection;
uniform mat4 view;

void main() {
    fWorldPos = aPosition;
    gl_Position = projection * view * vec4(fWorldPos, 1.0f);
}