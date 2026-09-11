#pragma once

#include <vector>

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
    std::vector<Vertex> m_vertices;
    std::vector<uint32_t> m_indices;

    // opengl objects
    GLuint m_vao = 0;
    GLuint m_vbo = 0;
    GLuint m_ebo = 0;
};