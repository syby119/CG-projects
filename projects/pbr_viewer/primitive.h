#pragma once
#include <cstdint>
#include "material.h"

struct Primitive {
    GLuint vertexArray;

    uint32_t firstVertex;
    uint32_t vertexCount;

    uint32_t firstIndex;
    uint32_t indexCount;

    PbrMaterial* material;
};