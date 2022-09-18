#version 330 core

out vec4 outColor;

in vec3 fWorldPos;

uniform float lod;
uniform samplerCube environmentMap;

// From http://filmicworlds.com/blog/filmic-tonemapping-operators/
vec3 uncharted2Tonemap(vec3 color) {
	float A = 0.15f;
	float B = 0.50f;
	float C = 0.10f;
	float D = 0.20f;
	float E = 0.02f;
	float F = 0.30f;
	float W = 11.2f;
	return ((color * (A * color + C * B) + D * E) / (color * (A * color + B) + D * F)) - E / F;
}

void main() {
    vec3 color = textureLod(environmentMap, fWorldPos, lod).rgb;
    color = uncharted2Tonemap(color);
    color = pow(color, vec3(1.0f / 2.2f));

    outColor = vec4(color, 1.0f);
}