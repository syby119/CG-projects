#include <stdio.h>

typedef struct Vector3f {
    float x, y, z;
} Vector3f;

Vector3f add(const Vector3f* lhs, const Vector3f* rhs) {
    Vector3f result;
    result.x = lhs->x + rhs->x;
    result.y = lhs->y + rhs->y;
    result.z = lhs->z + rhs->z;

    return result;
}

void print(const Vector3f* v) {
    printf("(%.3f, %.3f, %.3f)", v->x, v->y, v->z);
}

int main() {
    Vector3f v1 = { 1.0f, 0.0f, 0.0f };
    Vector3f v2 = { 0.0f, 1.0f, 0.0f };

    Vector3f v = add(&v1, &v2);

    print(&v1);
    printf(" + ");
    print(&v2);
    printf(" = ");
    print(&v);
    printf("\n");

    return 0;
}