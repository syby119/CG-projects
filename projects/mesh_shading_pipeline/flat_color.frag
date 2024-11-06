#version 450 core

out vec4 fragColor;
        
in PerVertexData {
    vec3 color;
} fragIn;

void main() {
    fragColor = vec4(fragIn.color, 1.0);
}