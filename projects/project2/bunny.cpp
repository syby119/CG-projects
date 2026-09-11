#include "bunny.h"
#include <glm/ext.hpp>

Bunny::Bunny(const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices)
    : m_vertices(vertices), m_indices(indices) {

    // create a vertex array object
    glGenVertexArrays(1, &m_vao);
    // create a vertex buffer object
    glGenBuffers(1, &m_vbo);
    // create a element array buffer
    glGenBuffers(1, &m_ebo);

    glBindVertexArray(m_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(
        GL_ARRAY_BUFFER, sizeof(Vertex) * m_vertices.size(), m_vertices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ebo);
    glBufferData(
        GL_ELEMENT_ARRAY_BUFFER, m_indices.size() * sizeof(uint32_t), m_indices.data(),
        GL_STATIC_DRAW);

    // specify layout, size of a vertex, data type, normalize, sizeof vertex array, offset of the
    // attribute
    glVertexAttribPointer(
        0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, position));
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(
        1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));
    glEnableVertexAttribArray(1);
    // glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex,
    // texCoord)); glEnableVertexAttribArray(2);

    glBindVertexArray(0);
}

Bunny::~Bunny() {
    if (m_ebo != 0) {
        glDeleteBuffers(1, &m_ebo);
        m_ebo = 0;
    }

    if (m_vbo != 0) {
        glDeleteBuffers(1, &m_vbo);
        m_vbo = 0;
    }

    if (m_vao != 0) {
        glDeleteVertexArrays(1, &m_vao);
        m_vao = 0;
    }
}

Bunny::Bunny(Bunny&& rhs) noexcept {
    m_vertices = std::move(rhs.m_vertices);
    m_indices = std::move(rhs.m_indices);

    m_vao = rhs.m_vao;
    m_vbo = rhs.m_vbo;
    m_ebo = rhs.m_ebo;

    rhs.m_vao = 0;
    rhs.m_vbo = 0;
    rhs.m_ebo = 0;
}

void Bunny::draw() {
    glBindVertexArray(m_vao);
    glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(m_indices.size()), GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}