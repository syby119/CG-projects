#version 330 core
layout(location = 0) out vec4 brightColorMap;

uniform sampler2D sceneMap;

in vec2 screenTexCoord;

void main() {
    // TODO 
    brightColorMap = vec4(0.0f, 0.0f, 0.0f, 1.0f);
}
