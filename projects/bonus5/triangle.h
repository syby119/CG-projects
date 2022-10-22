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

    bool intersect(const Ray& ray) {
        glm::vec3 o = ray.o;
        glm::vec3 dir = ray.dir;
        Vertex v1 = vertices[v[0]];
        Vertex v2 = vertices[v[1]];
        Vertex v3 = vertices[v[2]];

        glm::vec3 p1 = v1.position;
        glm::vec3 p2 = v2.position;
        glm::vec3 p3 = v3.position;
        glm::vec3 n1 = v1.normal;
        glm::vec3 n2 = v2.normal;
        glm::vec3 n3 = v3.normal;

        glm::vec3 e1 = p2 - p1;
        glm::vec3 e2 = p3 - p1;
        glm::vec3 n = glm::normalize(glm::cross(e1, e2));
        if (std::abs(glm::dot(n, dir)) < 1e-4f) {
            return false;
        }
        
        float d = glm::dot(n, p1);
        float tHit = (d - glm::dot(n, o)) / glm::dot(n, dir);
        if (tHit < 0 || tHit >= ray.tMax) { 
            return false;
        }

        glm::vec3 q = o + tHit * dir;
        float area = glm::dot(glm::cross(e1, e2), n);
        float alpha = glm::dot(glm::cross(p3 - p2, q - p2), n) / area;
        float beta = glm::dot(glm::cross(q - p1, p3 - p1), n) / area;
        float gamma = glm::dot(glm::cross(p2 - p1, q - p1), n) / area;
        
        if (alpha >= 0 && beta >= 0 && gamma >= 0) {
            ray.tMax = tHit;
            return true;
        } else {
            return false;
        }
    }

    static constexpr int getVertexTexDataComponent() noexcept {
        return 4;
    }

    static constexpr int getIndexTexDataComponent() noexcept {
        return 3;
    }
};