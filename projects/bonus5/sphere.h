#pragma once
#include "aabb.h"
#include <glm/glm.hpp>

struct Sphere {
public:
    glm::vec3 position;
    float radius;

public:
    Sphere() = default;

    Sphere(const glm::vec3& position, float radius) : position(position), radius(radius) {}

    static constexpr int getTexDataComponent() noexcept {
        return 4;
    }
};