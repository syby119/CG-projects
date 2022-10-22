#pragma once

#include <limits>
#include <glm/glm.hpp>

struct Ray {
public:
    glm::vec3 o;
    glm::vec3 dir;
    mutable float tMax;

public:
    Ray(): o(glm::vec3(0.0f)), dir(glm::vec3(0.0f)), tMax(std::numeric_limits<float>::max()) { }
    
    Ray(const glm::vec3& origin, glm::vec3& direction, float t = std::numeric_limits<float>::max())
        : o(origin), dir(direction), tMax(t) { }
    
    glm::vec3 operator()(float t) const {
        return o + t * dir;
    }
};