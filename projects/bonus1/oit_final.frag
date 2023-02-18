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
    float u = gl_FragCoord.x / float(windowExtent.width);
    float v = gl_FragCoord.y / float(windowExtent.height);
    vec4 frontColor = texture(blendTexture, vec2(u, v));
    color = vec4((frontColor + backgroundColor * frontColor.a).rgb, 1.0f);
}