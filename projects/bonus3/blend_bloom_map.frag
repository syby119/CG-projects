#version 330 core
out vec4 FragColor;

in vec2 screenTexCoord;

uniform sampler2D scene;
uniform sampler2D bloomBlur;

void main() {             
    vec3 sceneColor = texture(scene, screenTexCoord).rgb;      
    vec3 bloomColor = texture(bloomBlur, screenTexCoord).rgb;
    FragColor = vec4(sceneColor + bloomColor, 1.0);
}
