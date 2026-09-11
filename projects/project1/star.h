#pragma once

#include <glm/glm.hpp>
#include <vector>

#include "../base/gl_utility.h"

class Star {
public:
    Star(const glm::vec2& position, float rotation, float radius, float aspect);

    Star(const Star& rhs) = delete;

    Star(Star&& rhs) noexcept;

    ~Star();

    void draw() const;

private:
    glm::vec2 m_position;
    float m_rotation;
    float m_radius;

    GLuint m_vao = 0;
    GLuint m_vbo = 0;
    std::vector<glm::vec2> m_vertices;
};