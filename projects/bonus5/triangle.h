#pragma once

#include <glm/glm.hpp>

#include "../base/model.h"
#include "ray.h"
#include "aabb.h"

struct Triangle {
public:
    int v[3];
    Vertex* vertices;
public:
    Triangle() : vertices(nullptr) {
        v[0] = v[1] = v[2] = 0;
    }

    Triangle(int vi1, int vi2, int vi3, Vertex* vert = nullptr) : vertices(vert) {
        v[0] = vi1;
        v[1] = vi2;
        v[2] = vi3;
    }

    static constexpr int getVertexTexDataComponent() noexcept {
        return 4;
    }

    static constexpr int getIndexTexDataComponent() noexcept {
        return 3;
    }
};