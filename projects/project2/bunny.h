#pragma once

#include <vector>

#include <glm/glm.hpp>

#include "../base/gl_utility.h"
#include "../base/vertex.h"

class Bunny {
public:
    Bunny(const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices);

    ~Bunny();

    Bunny(const Bunny& rhs) = delete;

    Bunny(Bunny&& rhs) noexcept;

    void draw();

private:
    // vertices of the table represented in model's own coordinate
    std::vector<Vertex> _vertices;
    std::vector<uint32_t> _indices;

    // opengl objects
    GLuint _vao = 0;
    GLuint _vbo = 0;
    GLuint _ebo = 0;
};