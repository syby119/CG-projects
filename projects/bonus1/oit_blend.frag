#version 330 core
out vec4 color;

struct WindowExtent {
    int width;
    int height;
};

uniform WindowExtent windowExtent;
uniform sampler2D blendTexture;

void main() {
    float u = gl_FragCoord.x / float(windowExtent.width);
    float v = gl_FragCoord.y / float(windowExtent.height);
    color = texture(blendTexture, vec2(u, v));
}