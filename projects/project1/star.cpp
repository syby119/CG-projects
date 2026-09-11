#include "star.h"
#include <cmath>

Star::Star(const glm::vec2& position, float rotation, float radius, float aspect)
    : m_position(position), m_rotation(rotation), m_radius(radius) {
    // TODO: assemble the vertex data of the star
    // write your code here
    // -------------------------------------
    // for (int i = 0; i < 5; ++i) {
    //     m_vertices.push_back( ... );
    // }
    // -------------------------------------

    glGenVertexArrays(1, &m_vao);
    glGenBuffers(1, &m_vbo);

    glBindVertexArray(m_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(
        GL_ARRAY_BUFFER, sizeof(glm::vec2) * m_vertices.size(), m_vertices.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(glm::vec2), (void*)0);
    glEnableVertexAttribArray(0);

    glBindVertexArray(0);
}

Star::Star(Star&& rhs) noexcept
    : m_position(rhs.m_position), m_rotation(rhs.m_rotation), m_radius(rhs.m_radius),
      m_vao(rhs.m_vao), m_vbo(rhs.m_vbo) {
    rhs.m_vao = 0;
    rhs.m_vbo = 0;
}

Star::~Star() {
    if (m_vbo) {
        glDeleteVertexArrays(1, &m_vbo);
        m_vbo = 0;
    }

    if (m_vao) {
        glDeleteVertexArrays(1, &m_vao);
        m_vao = 0;
    }
}

void Star::draw() const {
    glBindVertexArray(m_vao);
    glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(m_vertices.size()));
}