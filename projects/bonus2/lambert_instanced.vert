#version 330 core
layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexture;
layout(location = 3) in mat4 aInstanceMatrix;

out vec3 fPosition;
out vec3 fNormal;
out vec2 fTexCoord;

uniform mat4 projection;
uniform mat4 view;

void main() {
    fPosition = vec3(aInstanceMatrix * vec4(aPosition, 1.0f));
    fNormal = mat3(transpose(inverse(aInstanceMatrix))) * aNormal;
    fTexCoord = aTexture;
    gl_Position = projection * view * aInstanceMatrix * vec4(aPosition, 1.0f);
}