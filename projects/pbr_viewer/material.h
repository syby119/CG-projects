#pragma once

#include <glm/glm.hpp>
#include <memory>

#include "../base/sampler.h"
#include "../base/texture.h"

struct Material {
    enum class AlphaMode {
        Blend,
        Opaque,
        Mask
    };

    enum AlphaMode alphaMode = AlphaMode::Opaque;
    float alphaCutoff = 1.0f;

    bool doubleSided = false;
};

struct PbrMaterial : public Material {
    std::string name;

    float roughnessFactor = 1.0f;
    float metallicFactor = 1.0f;
    float occlusionStrength = 1.0f;
    glm::vec4 albedoFactor = glm::vec4{1.0f};
    glm::vec4 emissiveFactor = glm::vec4{1.0f};

    Texture* albedoMap = nullptr;
    Texture* normalMap = nullptr;
    Texture* metallicMap = nullptr;
    Texture* roughnessMap = nullptr;
    Texture* occlusionMap = nullptr;
    Texture* emissiveMap = nullptr;

    Sampler* albeodoSampler = nullptr;
    Sampler* normalSampler = nullptr;
    Sampler* metallicSampler = nullptr;
    Sampler* roughnessSampler = nullptr;
    Sampler* occlusionSampler = nullptr;
    Sampler* emissiveSampler = nullptr;

    struct TexCoordSets {
        int albedo = -1;
        int metallic = -1;
        int roughness = -1;
        int normal = -1;
        int occlusion = -1;
        int emissive = -1;
    } texCoordSets;
};