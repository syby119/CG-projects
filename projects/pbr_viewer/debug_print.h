#pragma once

#include <iomanip>
#include <ostream>
#include <string>

#include "glm/ext.hpp"
#include "glm/glm.hpp"

inline std::string indent(int whitespaceCount) {
    return std::string(whitespaceCount, ' ');
}

inline std::ostream& operator<<(std::ostream& os, const glm::vec2& v) {
    os << std::fixed << std::setprecision(4);
    os << "(" << v[0] << ", " << v[1] << ")";
    return os << std::defaultfloat;
}

inline std::ostream& operator<<(std::ostream& os, const glm::vec3& v) {
    os << std::fixed << std::setprecision(4);
    os << "(" << v[0] << ", " << v[1] << ", " << v[2] << ")";
    return os << std::defaultfloat;
}

inline std::ostream& operator<<(std::ostream& os, const glm::vec4& v) {
    os << std::fixed << std::setprecision(4);
    os << "(" << v[0] << ", " << v[1] << ", " << v[2] << ", " << v[3] << ")";
    return os << std::defaultfloat;
}

inline std::ostream& operator<<(std::ostream& os, const glm::quat& q) {
    os << std::fixed << std::setprecision(4);
    os << "(" << q[0] << ", " << q[1] << ", " << q[2] << ", " << q[3] << ")";
    return os << std::defaultfloat;
}