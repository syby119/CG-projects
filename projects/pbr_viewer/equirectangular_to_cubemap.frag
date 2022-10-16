#version 330 core

out vec4 outColor; 

in vec3 fWorldPos;

uniform sampler2D equirectangularMap;

vec2 sampleSphericalMap(vec3 direction) {
    vec2 uv = vec2(atan(direction.z, direction.x), asin(direction.y));
    uv = uv * vec2(0.1591f, 0.3183f) + vec2(0.5f);

    return uv;
}

void main() {
    vec2 uv = sampleSphericalMap(normalize(fWorldPos));
    vec3 color = texture(equirectangularMap, uv).rgb;
    
    outColor = vec4(color, 1.0f);
}