#version 330 core
layout (location = 0) out vec4 fragColor;
layout (location = 1) out uint fragRngState;

in vec2 screenTexCoord;

const int LAMBERTIAN_MATERIAL = 0;
const int METAL_MATERIAL = 1;
const int DIELECTRIC_MATERIAL = 2;

const int SPHERE_SHAPE = 0;
const int TRIANGLE_SHAPE = 1;

const int BVH_INTERIOR_NODE = 0;
const int BVH_LEAF_NODE = 1;

const int DATA_BUFFER_WIDTH = 2048;

const float INFINITY = 10e10f;
const float FloatOneMinusEpsilon = 0.99999994f;
const float Pi = 3.14159265358979323846f;
const float Epsilon = 1e-3f;

const int windowWidth = 1200;
const int maxTraceDepth = 16;

struct Camera {
    mat4 cameraToWorld; //  the inverse matrix of view matrix
    mat4 rasterToCamera;
};

struct Ray {
    vec3 o;
    vec3 dir;
    float tMax;
};

struct Vertex {
    vec3 position;
    vec3 normal;
    vec2 texCoord;
};

struct LocalCoord {
    vec3 s;
    vec3 t;
    vec3 n;
};

struct AABB {
    vec3 pMin;
    vec3 pMax;
};

struct Sphere {
    vec3 position;
    float radius;
};

struct TriangleIndex {
    int v[3];
};

struct Triangle {
    Vertex v[3];
};

struct Material {
    int type;
    float fuzz;
    float ior;
    vec3 albedo;
};

struct Primitive {
    int shapeType; // 0: sphere 1 : triangle
    int shapeIdx;
    int materialIdx;
};

struct Interaction {
    Primitive primitive;
    Vertex hitPoint;
    Material material;
};

struct BVHNode {
    AABB box;
    int nodeType;
    int firstVal;  // leftChild / firstChildIndex
    int secondVal; // rightChild / nPrimitives
};

uniform sampler2D RTResult;
uniform usampler2D oldRngState;

uniform uint totalSamples;
uniform int nPrimitives;

// Scene Config 
uniform samplerCube sky;
uniform sampler2D sphereBuffer;
uniform sampler2D vertexBuffer;
uniform isampler2D triangleIndexBuffer;
uniform sampler2D materialBuffer;
uniform isampler2D primitiveBuffer;
uniform sampler2D bvh;

uniform Camera camera;

// tracer

/**
 * Summary: initialize the origin and direction of the ray of current pixel
 * Parameters:
 *     u: random offset of the origin
 * Return: the ray of current pixel
 */
Ray generateRay(vec2 u);

/**
 * Summary: get the hit point
 * Parameters:
 *     ray: 
 *     t  : distance between the origin and the hit point
 * Return: the hit point
 */
vec3 getHitPoint(inout Ray ray, float t);

/**
 * Summary: trace the ray in the scene and calculate the color of pixel
 * Parameters:
 *     ray: the ray
 * Return: the color of pixel
 */
vec4 trace(inout Ray ray);

vec3 gammaCorrection(vec3 color);
vec3 inverseGammaCorrection(vec3 color);
void outputSample(vec4 color);

// intersect
bool solveQuadraticEquation(float a, float b, float c, out float x1, out float x2);

/**
 * Summary: check whether the ray hit something
 * Parameters:
 *     ray: the ray
 *     isect: record the shape and material of the primitive
 * Return: true if the ray hit something
 */
bool intersect(inout Ray ray, inout Interaction isect);

/**
 * Summary: check whether the ray hit AABB
 * Parameters:
 *     ray: the ray
 *     box: the AABB that will be checked
 *     isect: record the shape and material of the primitive
 *     invDir: vec3(1.0f / ray.dir.x, 1.0f / ray.dir.y, 1.0f / ray.dir.z)
 * Return: true if the ray hit AABB
 */
bool intersectAABB(inout Ray ray, inout AABB box, inout vec3 invDir);

/**
 * Summary: check whether the ray hit primitive
 * Parameters:
 *     ray: the ray
 *     primitive: the primitive that will be checked
 *     isect: record the shape of the primitive
 * Return: true if the ray hit primitive
 */
bool intersectPrimitive(inout Ray ray, inout Primitive primitive, inout Interaction isect);

/**
 * Summary: check whether the ray hit sphere
 * Parameters:
 *     ray: the ray
 *     sphere: the sphere that will be checked
 *     isect: record the shape of the sphere
 * Return: true if the ray hit sphere
 */

bool intersectSphere(inout Ray ray, inout Sphere sphere, inout Interaction isect);

/**
 * Summary: check whether the ray hit triangle
 * Parameters:
 *     ray: the ray
 *     mesh: the triangle that will be checked
 *     isect: record the shape of the triangle
 * Return: true if the ray hit triangle
 */
bool intersectTriangle(inout Ray ray, inout Triangle mesh, inout Interaction isect);

// sample
/**
*Summary: uniform sample in the hemisphere
*Parameters:
*    u: random number vector
*Return: the sample vector
*/
vec3 uniformSampleHemiSphere(vec2 u);

/**
 * Summary: cosine-weighted sample in the hemisphere
 * Parameters:
 *     u: random number vector
 * Return: the sample vector
 */
vec3 cosineWeightedSampleHeimiSphere(vec2 u);

/**
 * Summary: uniform sample in the sphere
 * Parameters:
 *     u: random number vector
 * Return: the sample vector
 */
vec3 uniformSampleSphere(vec2 u);

// material
/**
 * Summary: approximate the reflectance of dielectric material
 * Parameters:
 *     cosTheta: cosine value between in vector and surface normal
 *     ior : index of refraction
 * Return: the reflectance of dielectric material
 */
float fresnelSchlick(float cosTheta, float ior);

/**
 * Summary: update the ray when hit lambertian material
 * Parameters:
 *     ray: the ray
 *     isect : record the shape and material of the primitive
 * Return: true if the ray continue bounce
 */
bool lambertianScatterFunction(inout Ray ray, inout Interaction isect);

/**
 * Summary: update the ray when hit metal material
 * Parameters:
 *     ray: the ray
 *     isect : record the shape and material of the primitive
 * Return: true if the ray continue bounce
 */
bool metalScatterFunction(inout Ray ray, inout Interaction isect);

/**
 * Summary: update the ray when hit dielectric material
 * Parameters:
 *     ray: the ray
 *     isect : record the shape and material of the primitive
 */
void dielectricScatterFunction(inout Ray ray, inout Interaction isect);

// random number generator
uint rngState;
void rngInit();
uint rngUpdateState();
uint rngXorShift();

/**
 * Summary: get random number in [0, 1)
 * Return: the random number
 */
float rngGetRandom1D();

/**
 * Summary: get random vector in [0, 1)
 * Return: the random vector
 */
vec2 rngGetRandom2D();

LocalCoord createLocalCoord(inout vec3 n);
vec3 toLocal(LocalCoord coord, vec3 v);
vec3 toWorld(LocalCoord coord, vec3 v);

void swap(inout float a, inout float b);
vec2 getSampleIdx(sampler2D data, int idx);
void getVec3FromTexture(sampler2D data, vec2 texCoord, out vec3 v);
void getVec4FromTexture(sampler2D data, vec2 texCoord, out vec4 v);

/**
 * Summary: get material data
 * Parameters:
 *     data: buffer of data
 *     idx : index of data
 *     material: store the data
 * Return: the material data
 * Usage: getMaterialData(materialBuffer, isect.primitive.materialIdx, isect.material)
 */
void getMaterialData(sampler2D data, int idx, out Material material);

/**
 * Summary: get sphere data
 * Parameters:
 *     data: buffer of data
 *     idx : index of data
 *     sphere: store the data
 * Return: the sphere data
 * Usage: getSphereData(sphereBuffer, isect.primitive.shapeIdx, sphere)
 */
void getSphereData(sampler2D data, int idx, out Sphere sphere);

/**
 * Summary: get the vertex index data of triangle
 * Parameters:
 *     data: buffer of data
 *     idx : index of data
 *     triIdx: store the data
 * Return: the vertex index data of triangle
 * Usage: getTriangleIndexData(triangleIndexBuffer, primitive.shapeIdx, idx)
 */
void getTriangleIndexData(isampler2D data, int idx, out TriangleIndex triIdx);

/**
 * Summary: get the vertex data of triangle
 * Parameters:
 *     data: buffer of data
 *     idx : index of data
 *     triIdx: store the data
 * Return: the vertex data of triangle
 * Usage: getTriangleData(vertexBuffer, triidx, triangle)
 */
void getTriangleData(sampler2D data, inout TriangleIndex idx, out Triangle triangle);

/**
 * Summary: get primitive data
 * Parameters:
 *     data: buffer of data
 *     idx : index of data
 *     triIdx: store the data
 * Return: the get primitive data
 * Usage: getPrimitiveData(primitiveBuffer, BVHNode.firstVal, primitive);
 */
void getPrimitiveData(isampler2D data, int idx, out Primitive primitive);

/**
 * Summary: get BVHNode data
 * Parameters:
 *     data: buffer of data
 *     idx : index of data
 *     triIdx: store the data
 * Return: the get BVHNode data
 * Usage: getBVHNodeData(bvh, BVHNode.firstVal or BVHNode.secondVal , node)
 */
void getBVHNodeData(sampler2D data, int idx, out BVHNode node);

void main() {
    rngInit();
    Ray ray = generateRay(vec2(rngGetRandom1D(), rngGetRandom1D())); 
    outputSample(trace(ray));
}

Ray generateRay(vec2 u) {
    vec4 pixelPos = vec4(gl_FragCoord.x - 0.5f + u.x, gl_FragCoord.y - 0.5f + u.y, 0.0f, 1.0f);
    Ray ray;
    ray.o = vec3(camera.cameraToWorld * vec4(0.0f, 0.0f, 0.0f, 1.0f));
    vec3 localRayDir = (camera.rasterToCamera * pixelPos).xyz - vec3(0.0f, 0.0f, 0.0f);
    ray.dir = normalize(vec3(camera.cameraToWorld * vec4(localRayDir.xyz, 0.0f)));
    ray.tMax = INFINITY;
    return ray;
}

vec3 getHitPoint(inout Ray ray, float t) {
    return ray.o + t * ray.dir;
}

vec4 trace(inout Ray ray) {
    // TODO: change the code here to trace the ray through the scene
    return vec4(texture(sky, ray.dir).rgb, 1.0f);
}

vec3 gammaCorrection(vec3 color) {
    return pow(color, vec3(1.0f / 2.2f));
}

vec3 inverseGammaCorrection(vec3 color) {
    return pow(color, vec3(2.2f));
}

void outputSample(vec4 color) {
    vec3 rst = texture(RTResult, screenTexCoord).rgb;
    fragColor = vec4(gammaCorrection(
        (inverseGammaCorrection(rst) * totalSamples + color.rgb) / (totalSamples + 1u)), 1.0f);
    fragRngState = rngState;
}

bool intersect(inout Ray ray, inout Interaction isect) {
    // TODO: perform ray hit the primitive with bvh trasversal
    return false;
}

bool intersectAABB(inout Ray ray, inout AABB box, inout vec3 invDir) {
    // TODO: perform ray hit AABB
    return false;
}

bool solveQuadraticEquation(float a, float b, float c, out float x1, out float x2) {
    // Find quadratic discriminant
    float discrim = b * b - 4 * a * c;
    if (discrim < 0) return false;
    float rootDiscrim = sqrt(discrim);

    // Compute quadratic _t_ values
    float q;
    if (b < 0)
        q = -.5 * (b - rootDiscrim);
    else
        q = -.5 * (b + rootDiscrim);
    x1 = float(q / a);
    x2 = float(c / q);
    if (x1 > x2) {
        swap(x1, x2);
    }
    return true;
}

bool intersectPrimitive(inout Ray ray, inout Primitive primitive, inout Interaction isect) {
    // TODO: perform ray hit the primitive, the type of the primitive can be
    //      + sphere    (use intersectSphere)
    //      + triangle  (use intersectTriangle)
    return false;
}

bool intersectSphere(inout Ray ray, inout Sphere sphere, inout Interaction isect) {
    // TODO: perform ray hit the sphere
    return false;
}

bool intersectTriangle(inout Ray ray, inout Triangle mesh, inout Interaction isect) {
    // TODO: perform ray hit the triangle
    return false;
}

// sample 
vec3 uniformSampleHemiSphere(vec2 u) {
    float z = 1 - u.x;
    float sinTheta = sqrt(max(0.0f, 1.0f - z * z));
    float phi = 2 * Pi * u.y;
    
    return vec3(cos(phi) * sinTheta, sin(phi) * sinTheta, z);
}

vec3 cosineWeightedSampleHeimiSphere(vec2 u) {
    float sinTheta = sqrt(u.x);
    float cosTheta = sqrt(1 - sinTheta * sinTheta);
    float phi = 2 * Pi * u.y;

    return vec3(cos(phi) * sinTheta, sin(phi) * sinTheta, cosTheta);
}

vec3 uniformSampleSphere(vec2 u) {
    float cosTheta = 1 - 2 * u.x;
    float sinTheta = sqrt(max(0.0f, 1 - cosTheta * cosTheta));
    float phi = 2 * Pi * u.y;

    return vec3(cos(phi) * sinTheta, sin(phi) * sinTheta, cosTheta);
}

// material
float fresnelSchlick(float cosTheta, float ior) {
    float r0 = (1.0f - ior) / (1.0f + ior);
    r0 = r0 * r0;

    return r0 + (1 - r0) * pow(1 - cosTheta, 5);
}

bool lambertianScatterFunction(inout Ray ray, inout Interaction isect) {
    // TODO: perform ray interact with the lambert material
    return false;
}

bool metalScatterFunction(inout Ray ray, inout Interaction isect) {
    // TODO: perform ray interact with the metallic material
    return false;
}

void dielectricScatterFunction(inout Ray ray, inout Interaction isect) { 
    // TODO: perform ray interact with the dielectric material
    return ;
}

void rngInit() {
    rngState = texture(oldRngState, screenTexCoord).r;
}

float rngGetRandom1D() {
    rngState ^= (rngState << 13);
    rngState ^= (rngState >> 17);
    rngState ^= (rngState << 5);
    return min(FloatOneMinusEpsilon, float(rngState) * (1.0 / 4294967296.0));
}

vec2 rngGetRandom2D() {
    return vec2(rngGetRandom1D(), rngGetRandom1D());
}

LocalCoord createLocalCoord(inout vec3 n) {
    LocalCoord res;
    res.n = normalize(n);
    if (abs(res.n.x) > abs(res.n.y)) {
        res.s = normalize(vec3(-res.n.z, 0.0f, res.n.x));
    }
    else {
        res.s = normalize(vec3(0.0f, -res.n.z, res.n.y));
    }
    res.t = normalize(cross(res.n, res.s));
    return res;
}

vec3 toLocal(LocalCoord coord, vec3 v) {
    return vec3(v.x * coord.s.x + v.y * coord.s.y + v.z * coord.s.z, 
                v.x * coord.t.x + v.y * coord.t.y + v.z * coord.t.z,
                v.x * coord.n.x + v.y * coord.n.y + v.z * coord.n.z);
}
vec3 toWorld(LocalCoord coord, vec3 v) {
    return v.x * coord.s + v.y * coord.t + v.z * coord.n;
}

void swap(inout float a, inout float b) {
    float tmp = a;
    a = b;
    b = tmp;
}

vec2 getSampleIdx(isampler2D data, int idx) {
    ivec2 texSize = textureSize(data, 0);
    int x = idx % texSize.x;
    int y = idx / texSize.x;
    return vec2((x + 0.5f) / texSize.x, (y + 0.5f) / texSize.y);
}

vec2 getSampleIdx(sampler2D data, int idx) {
    ivec2 texSize = textureSize(data, 0);
    int x = idx % texSize.x;
    int y = idx / texSize.x;
    return vec2((x + 0.5f) / texSize.x, (y + 0.5f) / texSize.y);
}
void getVec3FromTexture(sampler2D data, vec2 texCoord, out vec3 v) {
    v = texture(data, texCoord).rgb;
}

void getVec4FromTexture(sampler2D data, vec2 texCoord, out vec4 v) {
    v = texture(data, texCoord);
}

void getMaterialData(sampler2D data, int idx, out Material material) {
    idx *= 2;
    vec3 v;
    getVec3FromTexture(data, getSampleIdx(data, idx), v);
    material.type = int(v.x);
    material.ior = v.y;
    material.fuzz = v.z;
    getVec3FromTexture(data, getSampleIdx(data, idx + 1), material.albedo);
}

void getSphereData(sampler2D data, int idx, out Sphere sphere) {
    vec4 v;
    getVec4FromTexture(data, getSampleIdx(data, idx), v);
    sphere.position = v.xyz;
    sphere.radius = v.w;
}

void getTriangleIndexData(isampler2D data, int idx, out TriangleIndex triIdx) {
    ivec3 v = texture(data, getSampleIdx(data, idx)).xyz;
    triIdx.v[0] = v.x;
    triIdx.v[1] = v.y;
    triIdx.v[2] = v.z;
}

void getTriangleData(sampler2D data, inout TriangleIndex idx, out Triangle triangle) {
    vec4 v[2];
    for (int i = 0; i < 3; ++i) {
        int vid = idx.v[i] * 2;
        getVec4FromTexture(data, getSampleIdx(data, vid), v[0]);
        getVec4FromTexture(data, getSampleIdx(data, vid + 1), v[1]);
        triangle.v[i].position = v[0].xyz;
        triangle.v[i].normal = vec3(v[0].w, v[1].x, v[1].y);
        triangle.v[i].texCoord = v[1].zw;
    }
}

void getPrimitiveData(isampler2D data, int idx, out Primitive primitive) {
    ivec3 v = texture(data, getSampleIdx(data, idx)).xyz;
    primitive.shapeType = v.x;
    primitive.shapeIdx = v.y;
    primitive.materialIdx = v.z;
}

void getBVHNodeData(sampler2D data, int idx, out BVHNode node) {
    vec3 v[3];
    int vid = idx * 3;
    for (int i = 0; i < 3; ++i) {
        getVec3FromTexture(data, getSampleIdx(data, vid + i), v[i]);
    }
    node.box.pMin = v[0];
    node.box.pMax = v[1];
    node.nodeType = int(v[2].x);
    node.firstVal = int(v[2].y);
    node.secondVal = int(v[2].z);
}