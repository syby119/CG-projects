#version 330 core

#define M_PI 3.141592653589793

#define MAX_DIRECTIONAL_LIGHTS 4
#define MAX_POINT_LIGHTS 8
#define MAX_SPOT_LIGHTS 8

#define DEBUG_NONE      0
#define DEBUG_ALBEDO    1
#define DEBUG_ROUGHNESS 2
#define DEBUG_METALLIC  3
#define DEBUG_NORMAL    4
#define DEBUG_OCCLUSION 5
#define DEBUG_EMISSIVE  6

out vec4 outColor;

in vec3 fWorldPos;
in vec3 fNormal;
in vec2 fTexCoord0;
in vec2 fTexCoord1;

struct DirectionalLight {
    vec3 direction;
    vec3 color;
    float intensity;
};

struct PointLight {
    vec3 position;
    vec3 direction;
    vec3 color;
    vec3 intensity;
    float kc;
    float kl;
    float kq;
};

struct SpotLight {
    vec3 position;
    vec3 direction;
    vec3 color;
    float intensity;
    float kc;
    float kl;
    float kq;
    float angle;
};

layout(shared) uniform uboCamera {
    mat4 projection;
    mat4 view;
    vec3 viewPosition;
};

layout(shared) uniform uboLights {
    DirectionalLight directionalLights[MAX_DIRECTIONAL_LIGHTS];
    PointLight pointLights[MAX_POINT_LIGHTS];
    SpotLight spotLights[MAX_SPOT_LIGHTS];
    int directionalLightCount;
    int pointLightCount;
    int spotLightCount;
};

layout(shared) uniform uboEnvironment {
    float exposure;
    float gamma;
    uint maxPrefilterMipLevel;
    float scaleIBLAmbient;
};

struct Material {
    vec4 albedoFactor;
    vec4 emissiveFactor;
    float metallicFactor;
    float roughnessFactor;
    float occlusionStrength;
    
    int albedoTexCoordSet;
    int metallicTexCoordSet;
    int roughnessTexCoordSet;
    int normalTexCoordSet;
    int emissiveTexCoordSet;
    int occlusionTexCoordSet;

    bool doubleSided;

    bool alphaMask;
    float alphaMaskCutoff;
};

// material related textures
uniform Material material;
uniform sampler2D albedoMap;
uniform sampler2D normalMap;
uniform sampler2D metallicMap;
uniform sampler2D roughnessMap;
uniform sampler2D emissiveMap;
uniform sampler2D occlusionMap;

// environment related textures
uniform samplerCube irradianceMap;
uniform samplerCube prefilterMap;
uniform sampler2D brdfLutMap;

// for debug usage
uniform int debugInput;

struct PBRInfo {
    float NdotL;
    float NdotV;
    float NdotH;
    float LdotH;
    float VdotH;
    float perceptualRoughness;
    float metalness;
    vec3 reflectance0;
    vec3 reflectance90;
    float alphaRoughness;
    vec3 kd;
    vec3 ks;
};

// https://gamedev.stackexchange.com/questions/92015/optimized-linear-to-srgb-glsl
vec4 sRGBToLinear(vec4 color) {
    bvec3 cutoff = lessThan(color.rgb, vec3(0.04045f));
    vec3 higher = pow((color.rgb + vec3(0.055f))/vec3(1.055f), vec3(2.4f));
    vec3 lower = color.rgb / vec3(12.92f);

    return vec4(mix(higher, lower, cutoff), color.a);
}

// https://gamedev.stackexchange.com/questions/92015/optimized-linear-to-srgb-glsl
vec4 linearTosRGB(vec4 color) {
    bvec3 cutoff = lessThan(color.rgb, vec3(0.0031308f));
    vec3 higher = vec3(1.055f) * pow(color.rgb, vec3(1.0f / 2.4f)) - vec3(0.055f);
    vec3 lower = color.rgb * vec3(12.92f);

    return vec4(mix(higher, lower, cutoff), color.a);
}

vec2 getTexCoord(int texCoordSet) {
    return texCoordSet == 0 ? fTexCoord0 : fTexCoord1;
}

// Perturb normal, see http://www.thetenthplanet.de/archives/1180
vec3 getNormal() {
    vec2 texCoord = getTexCoord(material.normalTexCoordSet);
    vec3 tangentNormal = texture(normalMap, texCoord).xyz * 2.0f - 1.0f;

    vec3 q1 = dFdx(fWorldPos);
    vec3 q2 = dFdy(fWorldPos);
    vec2 st1 = dFdx(fTexCoord0);
    vec2 st2 = dFdy(fTexCoord0);

    vec3 N = normalize(fNormal);
    vec3 T = normalize(q1 * st2.t - q2 * st1.t);
    vec3 B = -normalize(cross(N, T));
    mat3 TBN = mat3(T, B, N);

    return normalize(TBN * tangentNormal);
}

// The following equation(s) model the distribution of microfacet normals across the area being drawn (aka D())
// Implementation from "Average Irregularity Representation of a Roughened Surface for Ray R" by T. S. Trowbridge, and K. P. Reitz
// Follows the distribution function recommended in the SIGGRAPH 2013 course notes from EPIC Games [1], Equation 3.
float getDistributionTerm(PBRInfo info) {
    float a2 = info.alphaRoughness * info.alphaRoughness;
    float NdotH = info.NdotH;
    float f = (NdotH * a2 - NdotH) * NdotH + 1.0;
    return a2 / (M_PI * f * f);
}

// the separable form of the Smith joint masking-shadowing function
float getGeometryTerm(PBRInfo info) {
    float NdotL = info.NdotL;
    float NdotV = info.NdotV;
    float a2 = info.alphaRoughness * info.alphaRoughness;
    float attenuationL = 2.0f * NdotL / (NdotL + sqrt(a2 + (1.0 - a2) * (NdotL, NdotL)));
    float attenuationV = 2.0f * NdotV / (NdotV + sqrt(a2 + (1.0 - a2) * (NdotV, NdotV)));

    return attenuationL * attenuationV;
}

// kind of schlick approximation
vec3 getFresnelTerm(PBRInfo info) {
    return info.reflectance0 + 
        (info.reflectance90 - info.reflectance0) * pow(1.0f - info.VdotH, 5.0f);
}

vec3 getBRDF(PBRInfo info) {
    vec3 F = getFresnelTerm(info);
    float G = getGeometryTerm(info);
    float D = getDistributionTerm(info);

    vec3 diffuseBRDF = (1.0f - F) * info.kd / M_PI;
    vec3 specularBRDF = F * G * D / (4.0f * info.NdotL * info.NdotV);

    return diffuseBRDF + specularBRDF;
}

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

vec4 tonemap(vec4 color) {
    vec3 result = uncharted2Tonemap(color.rgb * exposure);
    result *= (1.0f / uncharted2Tonemap(vec3(11.2f)));
    return vec4(pow(result, vec3(1.0f / gamma)), color.a);
}

vec3 getIBLTerm(PBRInfo info, vec3 N, vec3 R) {
    vec3 diffuse = info.kd * texture(irradianceMap, N).rgb;

    float lod = info.perceptualRoughness * maxPrefilterMipLevel;
    vec2 brdf = texture(brdfLutMap, vec2(info.NdotV, info.perceptualRoughness)).rg;
    vec3 specular = textureLod(prefilterMap, R, lod).rgb * (info.ks * brdf.x + brdf.y);

    return (diffuse + specular) * scaleIBLAmbient;
}

void main() {
    // albedo
    vec4 albedo = material.albedoFactor;
    if (material.albedoTexCoordSet >= 0) {
        albedo *= sRGBToLinear(texture(albedoMap, getTexCoord(material.albedoTexCoordSet)));
    }

    // alpha mask
    if (material.alphaMask == true && albedo.a < material.alphaMaskCutoff) {
        discard;
    }

    // roughness: sample texture in g channel
    float perceptualRoughness = material.roughnessFactor;
    if (material.roughnessTexCoordSet >= 0) {
        perceptualRoughness *= texture(roughnessMap, getTexCoord(material.roughnessTexCoordSet)).g;
    }

    // metallic: sample texture in b channel 
    float metallic = material.metallicFactor;
    if (material.metallicTexCoordSet >= 0) {
        metallic *= texture(metallicMap, getTexCoord(material.metallicTexCoordSet)).b;
    }

    float alphaRoughness = perceptualRoughness * perceptualRoughness;

    vec3 F0 = vec3(0.04f);
    vec3 kd = albedo.rgb * (vec3(1.0f) - F0) * (1.0f - metallic);
    vec3 ks = mix(F0, albedo.rgb, metallic);

    float reflectance = max(max(ks.r, ks.g), ks.b);
    vec3 reflectance0 = ks.rgb;
    vec3 reflectance90 = vec3(clamp(reflectance * 25.0f, 0.0f, 1.0f));

    vec3 N = material.normalTexCoordSet >= 0 ? getNormal() : normalize(fNormal);
    if (material.doubleSided && gl_FrontFacing == false) {
        N = -N;
    }

    vec3 V = normalize(viewPosition - fWorldPos);
    float NdotV = clamp(abs(dot(N, V)), 0.001f, 1.0f);

    PBRInfo info = PBRInfo(0.0f, NdotV, 0.0f, 0.0f, 0.0f, 
        perceptualRoughness, metallic, reflectance0, reflectance90, alphaRoughness, kd, ks);

    vec3 accumColor = vec3(0.0f);
    // get color from directional lights
    for (int i = 0; i < directionalLightCount; ++i) {
        vec3 L = normalize(-directionalLights[i].direction);
        vec3 H = normalize(L + V);

        info.NdotL = clamp(dot(N, L), 0.001f, 1.0f);
        info.NdotH = clamp(dot(N, H), 0.000f, 1.0f);
        info.LdotH = clamp(dot(L, H), 0.000f, 1.0f);
        info.VdotH = clamp(dot(V, H), 0.000f, 1.0f);

        vec3 brdf = getBRDF(info);

        vec3 Li = directionalLights[i].color * directionalLights[i].intensity;

        accumColor += brdf * Li * info.NdotL;
    }

    // get color from point lights
    for (int i = 0; i < pointLightCount; ++i) {
        vec3 d = spotLights[i].position - fWorldPos;
        vec3 L = normalize(d);
        vec3 H = normalize(L + V);

        info.NdotL = clamp(dot(N, L), 0.001f, 1.0f);
        info.NdotH = clamp(dot(N, H), 0.000f, 1.0f);
        info.LdotH = clamp(dot(L, H), 0.000f, 1.0f);
        info.VdotH = clamp(dot(V, H), 0.000f, 1.0f);

        vec3 brdf = getBRDF(info);

        float dist = length(d);
        float attenuation = 1.0f / 
            (pointLights[i].kc + pointLights[i].kl * dist + pointLights[i].kq * dist * dist);
        vec3 Li = pointLights[i].color * pointLights[i].intensity * attenuation;
        
        accumColor += brdf * Li * info.NdotL;
    }

    // get color from spot lights
    for (int i = 0; i < spotLightCount; ++i) {
        vec3 d = spotLights[i].position - fWorldPos;
        vec3 L = normalize(d);

        // test cut off 
        float angle = acos(-dot(L, spotLights[i].direction));
        if (angle > spotLights[i].angle) {
            continue;
        }

        vec3 H = normalize(L + V);

        info.NdotL = clamp(dot(N, L), 0.001f, 1.0f);
        info.NdotH = clamp(dot(N, H), 0.000f, 1.0f);
        info.LdotH = clamp(dot(L, H), 0.000f, 1.0f);
        info.VdotH = clamp(dot(V, H), 0.000f, 1.0f);

        vec3 brdf = getBRDF(info);

        float dist = length(d);
        float attenuation = 1.0f / 
            (spotLights[i].kc + spotLights[i].kl * dist + spotLights[i].kq * dist * dist);
        vec3 Li = spotLights[i].color * spotLights[i].intensity * attenuation;
        
        accumColor += brdf * Li * info.NdotL;
    }

    // IBL contribution
    vec3 R = normalize(reflect(-V, N));
    accumColor += getIBLTerm(info, N, R);

    // occlusion: sample texture in r channel
    if (material.occlusionTexCoordSet >= 0) {
        accumColor *= (1.0f + material.occlusionStrength * 
            (texture(occlusionMap, getTexCoord(material.occlusionTexCoordSet)).r - 1.0f));
    }

    // emissive
    vec4 emissive = material.emissiveFactor;
    if (material.emissiveTexCoordSet >= 0) {
        emissive *= sRGBToLinear(texture(emissiveMap, getTexCoord(material.emissiveTexCoordSet)));
    }
    accumColor += emissive.rgb;

    accumColor = pow(accumColor, vec3(1.0f / 2.2f));
    outColor = vec4(accumColor, albedo.a);

    switch (debugInput) {
        case DEBUG_ALBEDO:
            outColor = material.albedoFactor;
            if (material.albedoTexCoordSet >= 0) {
                outColor *= texture(albedoMap, getTexCoord(material.albedoTexCoordSet));
            }
            break;
        case DEBUG_ROUGHNESS:
            outColor.rgb = vec3(material.roughnessFactor);
            if (material.roughnessTexCoordSet >= 0) {
                outColor.rgb *= texture(roughnessMap, getTexCoord(material.roughnessTexCoordSet)).ggg;
            }
            outColor.a = 1.0f;
            break;
        case DEBUG_METALLIC:
            outColor.rgb = vec3(material.metallicFactor);
            if (material.metallicTexCoordSet >= 0) {
                outColor.rgb *= texture(metallicMap, getTexCoord(material.metallicTexCoordSet)).bbb;
            }
            outColor.a = 1.0f;
            break;
        case DEBUG_NORMAL:
            if (material.normalTexCoordSet >= 0) {
                outColor.rgb = texture(normalMap, getTexCoord(material.metallicTexCoordSet)).rgb;
            } else {
                outColor.rgb = normalize(fNormal);
            }
            outColor.a = 1.0f;
            break;
        case DEBUG_OCCLUSION:
            outColor = vec4(0.0, 0.0, 0.0, 1.0);
            if (material.occlusionTexCoordSet >= 0) {
                outColor.rgb = texture(occlusionMap, getTexCoord(material.occlusionTexCoordSet)).rrr;
            }
            break;
        case DEBUG_EMISSIVE:
            outColor = material.emissiveFactor;
            if (material.emissiveTexCoordSet >= 0) {
                outColor *= sRGBToLinear(texture(emissiveMap, getTexCoord(material.emissiveTexCoordSet)));
            }
            outColor.a = 1.0f;
            break;
        default:
            break;
    }

}