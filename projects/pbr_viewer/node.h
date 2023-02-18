#pragma once

#include <memory>
#include <vector>
#include <glm/glm.hpp>
#include <glm/ext.hpp>

#include "../base/transform.h"
#include "primitive.h"

struct Node {
    std::string name;
    int index = -1;
    Node* parent = nullptr;
    std::vector<Node*> children;
    Transform transform;
    std::vector<Primitive> primitives;
};