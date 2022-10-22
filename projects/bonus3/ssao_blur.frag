#version 330 core
layout(location = 0) out float blurResult;

uniform sampler2D ssaoResult;

in vec2 screenTexCoord;

void main() {
    vec2 texelSize = 1.0 / vec2(textureSize(ssaoResult, 0));
    float result = 0.0;
    for (int x = -2; x <= 2; ++x) {
        for (int y = -2; y <= 2; ++y) {
            vec2 offset = vec2(float(x), float(y)) * texelSize;
            result += texture(ssaoResult, screenTexCoord + offset).r;
        }
    }

    blurResult = result / (5.0 * 5.0);
}