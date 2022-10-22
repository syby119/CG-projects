#pragma once

#include <algorithm>
#include <limits>
#include <glm/glm.hpp>

#include "ray.h"

struct AABB {
public:
    glm::vec3 pMin;
    glm::vec3 pMax;

public:
    AABB() : pMin(glm::vec3(std::numeric_limits<float>::max())),
             pMax(-glm::vec3(std::numeric_limits<float>::max())) { }

    AABB(const glm::vec3& p1, const glm::vec3& p2) : 
        pMin(glm::min(p1, p2)),
        pMax(glm::max(p1, p2)) { }

    glm::vec3& operator[](int i) {
        return i == 0 ? pMin : pMax;
    }

    const glm::vec3& operator[](int i) const {
        return i == 0 ? pMin : pMax;
    }

    glm::vec3 corner(int i) const {
        return glm::vec3(
            (i & 1) ? pMin.x : pMax.x,
            (i & 2) ? pMin.y : pMax.y,
            (i & 4) ? pMin.z : pMax.z
        );
    }

    float surfaceArea() const {
        glm::vec3 diagonal = pMax - pMin;
        return 2 * (diagonal.x * diagonal.y + diagonal.x * diagonal.z + diagonal.y * diagonal.z);
    }
};

inline AABB unionAABB(const AABB& box1, const AABB& box2) {
    return AABB(glm::min(box1.pMin, box2.pMin), glm::max(box1.pMax, box2.pMax));
}

inline AABB unionAABB(const AABB& box, const glm::vec3& p) {
    return AABB(glm::min(box.pMin, p), glm::max(box.pMax, p));
}

inline int maximumDim(const AABB& box) {
    glm::vec3 dis = box.pMax - box.pMin;
    if (dis.x > dis.y) {
        return dis.x > dis.z ? 0 : 2;
    } else {
        return dis.y > dis.z ? 1 : 2;
    }
}