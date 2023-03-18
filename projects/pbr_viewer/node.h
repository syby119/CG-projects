#pragma once

#include <glm/ext.hpp>
#include <glm/glm.hpp>
#include <memory>
#include <vector>

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