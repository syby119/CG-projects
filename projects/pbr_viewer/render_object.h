#pragma once

#include "primitive.h"
#include <glm/glm.hpp>

struct RenderObject {
    glm::mat4 globalMatrix;
    const Primitive* primitive;
};