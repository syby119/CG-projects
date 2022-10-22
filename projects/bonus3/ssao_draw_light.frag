#version 330 core
layout(location = 0) out vec4 fragColor;

uniform vec3 lightColor;
uniform float lightIntensity;

void main() {
    fragColor = vec4(lightColor * lightIntensity, 1.0);
}