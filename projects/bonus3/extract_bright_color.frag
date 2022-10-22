#version 330 core
layout(location = 0) out vec4 brightColorMap;

uniform sampler2D sceneMap;

in vec2 screenTexCoord;

void main() {
    // TODO: extract the bright color with color components greater than 1.0
    brightColorMap = vec4(0.0f, 0.0f, 0.0f, 1.0f);
}
