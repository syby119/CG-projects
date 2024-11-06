#version 430 core

layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoord;
layout(location = 3) in mat4 aInstanceMatrix;

uniform mat4 viewProjection;

out PerVertexData {
    vec3 positionWS;
    vec3 normalWS;
    vec2 texCoord;
} vertOut;

void main() {
    vec4 positionWS = aInstanceMatrix * vec4(aPosition, 1.0f);

    vertOut.positionWS = vec3(positionWS);
    vertOut.normalWS   = mat3(transpose(inverse(aInstanceMatrix))) * aNormal;
    vertOut.texCoord  = aTexCoord;

    gl_Position = viewProjection * positionWS;
};