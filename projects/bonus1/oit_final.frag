#version 330 core
out vec4 color;

struct WindowExtent {
    int width;
    int height;
};

uniform WindowExtent windowExtent;
uniform sampler2D blendTexture;
uniform vec4 backgroundColor;

void main() {
    float u = gl_FragCoord.x / windowExtent.width;
    float v = gl_FragCoord.y / windowExtent.height;
    vec4 frontColor = texture(blendTexture, vec2(u, v));
    color = frontColor + backgroundColor * frontColor.a;
}