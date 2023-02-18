#version 330 core
layout(location = 0) in vec3 aPosition;
layout(location = 1) in mat4 aInstanceMatrix;
flat out int visible;

struct BoundingBox {
    vec3 min;
    vec3 max;
};

struct Plane {
    vec3 normal;
    float signedDistance;
};

struct Frustum {
    Plane planes[6];
};

uniform BoundingBox boundingBox;
uniform Frustum frustum;

// TODO: Modify the following code to achieve GPU frustum culling
void main() {
    visible = 1;
}