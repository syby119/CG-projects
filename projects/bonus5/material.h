#pragma once

#include <glm/glm.hpp>

struct Material {
public:
    enum class Type {
        Lambertian,
        Metal,
        Dielectric
    };

public:
    Type type;
    float ior;
    float fuzz;
    glm::vec3 albedo;

public:
    Material() = default;

    Material(Type type, float ior, float fuzz, const glm::vec3& albedo) :
        type(type),
        ior(ior),
        fuzz(fuzz), 
        albedo(albedo) {}

    static constexpr int getTexDataComponent() noexcept {
        return 3;
    }
};