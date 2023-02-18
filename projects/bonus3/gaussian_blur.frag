#version 330 core
out vec4 FragColor;

in vec2 screenTexCoord;

uniform sampler2D image;

uniform bool horizontal;

const float weight[5] = float[] (
    0.2270270270f, 0.1945945946f, 0.1216216216f, 0.0540540541f, 0.0162162162f);

void main() {
    // TODO: perform gaussian blur
    FragColor = vec4(texture(image, screenTexCoord).rgb, 1.0);
}