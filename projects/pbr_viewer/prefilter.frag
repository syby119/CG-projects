#version 400 core

#define M_PI 3.14159265359

out vec4 outColor;

in vec3 fWorldPos;

uniform uint numSamples;
uniform float roughness;
uniform samplerCube environmentMap;

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

float distributionGGX(float NdotH, float roughness) {
	float alpha = roughness * roughness;
	float alpha2 = alpha * alpha;
	float denom = NdotH * NdotH * (alpha2 - 1.0f) + 1.0f;

	return alpha2 / (M_PI * denom * denom); 
}

void main() {
    vec3 N = normalize(fWorldPos);

    vec3 R = N;
    vec3 V = R;

    float resolution = float(textureSize(environmentMap, 0).s);

    // https://placeholderart.wordpress.com/2015/07/28/
    vec3 accumColor = vec3(0.0f);
    float totalWeight = 0.0f;
    for (uint i = 0; i < numSamples; ++i) {
        vec2 Xi = hammersley(i, numSamples);
        vec3 H = importanceSampleGGX(Xi, N, roughness);
        vec3 L = normalize(2.0f * dot(V, H) * H - V);

        float NdotL = clamp(dot(N, L), 0.0f, 1.0f);
        if (NdotL > 0.0f) {
            float NdotH = clamp(dot(N, H), 0.0f, 1.0f);
            float HdotV = clamp(dot(H, V), 0.0f, 1.0f);
            float pdf = distributionGGX(NdotH, roughness) * NdotH / (4.0f * HdotV) + 0.0001f;

            float omageP = 4.0f * M_PI / (6.0f * resolution * resolution);
            float omageS = 1.0f / (float(numSamples) * pdf + 0.0001f);

            const float mipBias = 1.0f;
            float mipLevel = roughness == 0.0 ? 0.0 : max(0.5 * log2(omageS / omageP) + mipBias, 0.0f);

            accumColor += textureLod(environmentMap, L, mipLevel).rgb * NdotL;
            totalWeight += NdotL;
        }
    }

    outColor = vec4(accumColor / totalWeight, 1.0f);
}