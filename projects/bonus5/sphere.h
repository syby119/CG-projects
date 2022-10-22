#pragma once
#include <glm/glm.hpp>
#include "aabb.h"

struct Sphere {
public:
    glm::vec3 position;
    float radius;

public:
    Sphere() = default;

    Sphere(const glm::vec3& position, float radius) : 
        position(position), radius(radius) { }

    static constexpr int getTexDataComponent() noexcept {
        return 4;
    }
};