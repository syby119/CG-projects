#version 330 core
layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoord;

uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;

out vec3 position;
out vec3 normal;
out vec2 texCoord;

void main() {
    vec4 viewSpacePos = view * model * vec4(aPosition, 1.0f);
    position = viewSpacePos.xyz;
    normal = normalize(mat3(transpose(inverse(view * model))) * aNormal);
    texCoord = aTexCoord;
    gl_Position = projection * viewSpacePos;
}