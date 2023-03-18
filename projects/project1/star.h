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
    glm::vec2 _position;
    float _rotation;
    float _radius;

    GLuint _vao = 0;
    GLuint _vbo = 0;
    std::vector<glm::vec2> _vertices;
};