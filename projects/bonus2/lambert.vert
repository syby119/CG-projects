#version 330 core
layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoord;

out vec3 fPosition;
out vec3 fNormal;
out vec2 fTexCoord;

uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;

void main() {
    fPosition = vec3(model * vec4(aPosition, 1.0f));
    fNormal = mat3(transpose(inverse(model))) * aNormal;
    fTexCoord = aTexCoord;
    gl_Position = projection * view * model * vec4(aPosition, 1.0f);
}