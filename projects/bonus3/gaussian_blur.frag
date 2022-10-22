#version 330 core
out vec4 FragColor;

in vec2 screenTexCoord;

uniform sampler2D image;

uniform bool horizontal;
uniform float weight[5] = float[] (
    0.2270270270, 0.1945945946, 0.1216216216, 0.0540540541, 0.0162162162);

void main() {             
    // TODO: perform gaussian blur
    FragColor = vec4(texture(image, screenTexCoord).rgb, 1.0);
}