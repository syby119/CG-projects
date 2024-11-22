#version 430 core

layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoord;

layout(location = 0) out vec3 fPosition;
layout(location = 1) out vec3 fNormal;
layout(location = 2) out vec2 fTexCoord;

layout(std140, binding = 0) uniform UboCamera {
    mat4 projection;
    mat4 view;
    vec3 viewPosition;
};

layout(location = 0) uniform mat4 model;

void main() {
    fPosition = vec3(model * vec4(aPosition, 1.0f));
    fNormal = mat3(transpose(inverse(model))) * aNormal;
    fTexCoord = aTexCoord;
    gl_Position = projection * view * model * vec4(aPosition, 1.0f);
}