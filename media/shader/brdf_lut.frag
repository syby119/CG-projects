#version 330 core
#define M_PI 3.14159265359

in vec2 fTexCoord;

out vec2 outColor;

uniform uint numSamples;

// http://holger.dammertz.org/stuff/notes_HammersleyOnHemisphere.html
// efficient VanDerCorpus calculation.
vec2 hammersley(uint i, uint n) {
    uint bits = (i << 16u) | (i >> 16u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
    float rdi = float(bits) * 2.3283064365386963e-10; // divide 0x100000000

	return vec2(float(i)/float(n), rdi);
}

// http://blog.selfshadow.com/publications/s2013-shading-course/karis/s2013_pbs_epic_slides.pdf
vec3 importanceSampleGGX(vec2 Xi, vec3 N, float roughness) {
    float a = roughness * roughness;

    float phi = 2.0f * M_PI * Xi.x;
    float cosTheta = sqrt((1.0f - Xi.y) / (1.0f + (a * a - 1.0f) * Xi.y));
    float sinTheta = sqrt(1.0f - cosTheta * cosTheta);

    vec3 H = vec3(cos(phi) * sinTheta, sin(phi) * sinTheta, cosTheta);

    // Compute a tangent frame and rotate the half vector to world space
    vec3 up = abs(N.z) < 0.999f ? vec3(0.0f, 0.0f, 1.0f) : vec3(1.0f, 0.0f, 0.0f);
    vec3 tangent = normalize(cross(up, N));
    vec3 bitangent = cross(N, tangent);

    // Tangent to world space
    vec3 sampleVec = tangent * H.x + bitangent * H.y + N * H.z;
    return normalize(sampleVec);
}

float geometrySmithGGX(float NdotL, float NdotV, float roughness) {
	float k = (roughness * roughness) / 2.0;
	float ggxL = NdotL / (NdotL * (1.0 - k) + k);
	float ggxV = NdotV / (NdotV * (1.0 - k) + k);

	return ggxL * ggxV;
}

vec2 integrateBRDF(float NdotV, float roughness) {
	// Normal always points along z-axis for the 2D lookup 
	vec3 N = vec3(0.0f, 0.0f, 1.0f);
	vec3 V = vec3(sqrt(1.0f - NdotV * NdotV), 0.0f, NdotV);

	vec2 lut = vec2(0.0f);
	for (uint i = 0u; i < numSamples; ++i) {
		vec2 Xi = hammersley(i, numSamples);
		vec3 H = importanceSampleGGX(Xi, N, roughness);
		vec3 L = normalize(2.0f * dot(V, H) * H - V);

		float NdotL = max(L.z, 0.0f);
		float NdotH = max(H.z, 0.0f);
		float VdotH = max(dot(V, H), 0.0f);

		if (NdotL > 0.0f) {
			float G = geometrySmithGGX(NdotL, NdotV, roughness);
			float Fc = pow(1.0f - VdotH, 5.0f);

			lut += vec2(1.0f - Fc, Fc) * G * VdotH / (NdotH * NdotV);
		}
	}

	return lut / float(numSamples);
}


void main() {
	outColor = integrateBRDF(fTexCoord.x, fTexCoord.y);
}