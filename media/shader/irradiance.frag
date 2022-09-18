#version 330 core
#define M_PI    3.141592653589793

in vec3 fWorldPos;

out vec4 outColor;

uniform samplerCube environmentMap;
uniform float deltaPhi;
uniform float deltaTheta;

void main() {
    vec3 N = normalize(fWorldPos);

    vec3 up = vec3(0.0f, 1.0f, 0.0f);
    vec3 right = normalize(cross(up, N));
    up = normalize(cross(N, right));

    const float twoPI = 2.0f * M_PI;
    const float halfPI = 0.5f * M_PI;

    vec3 irradiance = vec3(0.0f);
    int samples = 0;
    for (float phi = 0.0f; phi < twoPI; phi += deltaPhi) {
        for (float theta = 0.0f; theta < halfPI; theta += deltaTheta) {
            float sinTheta = sin(theta);
            float cosTheta = cos(theta);

            vec3 tangentSample = vec3(sinTheta * cos(phi), sinTheta * sin(phi), cosTheta);
            vec3 samplerVec = tangentSample.x * right + tangentSample.y * up + tangentSample.z * N;

            irradiance += texture(environmentMap, samplerVec).rgb * cosTheta * sinTheta;
            ++samples;
        }
    }

    irradiance = M_PI * irradiance / float(samples);

    outColor = vec4(irradiance, 1.0f);
}