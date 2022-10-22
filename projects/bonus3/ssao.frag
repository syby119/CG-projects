#version 330 core
layout(location = 0) out float ssaoResult;

const int nSamples = 64;
const float radius = 1.0f;

uniform int screenWidth;
uniform int screenHeight;
uniform sampler2D gPosition;
uniform sampler2D gNormal;
uniform sampler2D noiseMap;
uniform vec3 sampleVecs[64];
uniform mat4 projection;

in vec2 screenTexCoord;

void main() {
    // TODO: perform SSAO
    ssaoResult = 1.0f;
}