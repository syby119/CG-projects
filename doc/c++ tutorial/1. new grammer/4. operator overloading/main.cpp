#include <iostream>

struct Vector3f {
    float x, y, z;
};

Vector3f operator+(const Vector3f& lhs, const Vector3f& rhs) {
    Vector3f result;
    result.x = lhs.x + rhs.x;
    result.y = lhs.y + rhs.y;
    result.z = lhs.z + rhs.z;

    return result;
}

std::ostream& operator<<(std::ostream& os, const Vector3f& v) {
    os << "(" << v.x << ", " << v.y << ", " << v.z << ")";
    return os;
}

int main() {
    Vector3f v1 = { 1.0f, 0.0f, 0.0f };
    Vector3f v2 = { 0.0f, 1.0f, 0.0f };

    Vector3f v = v1 + v2;

    std::cout << v1 << " + " << v2 << " = " << v << std::endl;

    return 0;
}