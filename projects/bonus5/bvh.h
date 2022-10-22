#pragma once 

#include <vector>
#include "aabb.h"
#include "ray.h"
#include "triangle.h"
#include "sphere.h"
#include "primitive.h"

struct Interaction {
    Primitive primitive;
    Vertex hitPoint;
    Material material;
};

struct PrimitiveInfo {
public:
    int pid;
    AABB box;
    glm::vec3 centroid;

public:
    PrimitiveInfo() = default;

    PrimitiveInfo(int idx, const AABB& bound) :
        box(bound), 
        centroid(0.5f * (bound.pMax + bound.pMin)),
        pid(idx) {}
};

struct BVHBuildNode {
public:
    AABB bound;
    BVHBuildNode* leftChild;
    BVHBuildNode* rightChild;
    int splitAxis, startIdx, nPrimitives;

public:
    BVHBuildNode() : 
        leftChild(nullptr), rightChild(nullptr), splitAxis(0), startIdx(0), nPrimitives(0) { }

    void initLeafNode(const AABB& box, int sid, int n) {
        bound = box;
        startIdx = sid;
        nPrimitives = n;
        leftChild = rightChild = nullptr;
    }

    void initInteriorNode(BVHBuildNode* left, BVHBuildNode* right, int axis) {
        leftChild = left;
        rightChild = right;
        splitAxis = axis;
        nPrimitives = 0;
        bound = unionAABB(left->bound, right->bound);
    }
};

struct BVHNode {
public:
    enum class Type { NonLeaf, Leaf };
public:
    AABB box;
    Type type;
    union {
        struct {
            int leftChild;
            int rightChild;
        };
        struct {
            int startIndex;
            int nPrimitives;
        };
    };

public:
    BVHNode() : type(Type::Leaf), leftChild(-1), rightChild(-1) { }

    static constexpr int getTexDataComponent() noexcept {
        return 3;
    }
};

class BVH {
public:
    std::vector<BVHNode> nodes;
    std::vector<Primitive> orderedPrimitives;
    int height;
    int maxHeight;

public:
    BVH(std::vector<Primitive>& primitives) {
        constructBVH(primitives);
    }

    bool intersect(const Ray& ray, Interaction& isect);

private:
    void constructBVH(std::vector<Primitive>& primitives);

    /*
    *Summary: build bvh and store primitives in orderedPrimitives
    *Parameters:
    *     primitives: the primitives in scene
    *     primInfo  : PrimitiveInfo of primitives
    *     start     : start index of primitives
    *     end       : end index of primitives
    *     totalNodes: the number of nodes was crated
    *Return: the root of BVH
    */
    BVHBuildNode* recursiveBuild(
        const std::vector<Primitive>& primitives,
        std::vector<PrimitiveInfo>& primInfo, 
        int start, int end, int* totalNodes);
    /*
    *Summary: convert BVH to array form
    *Parameters:
    *     root: root of bvh
    *     offset: offset of nodes array
    *Return: node index in nodes array
    */
    int toLinearTree(BVHBuildNode* root, int* offset);

    static AABB getAABB(const Primitive& prim);

    static AABB getTriangleAABB(const Triangle& triangle);

    static AABB getSphereAABB(const Sphere& sphere);

    static bool intersectPrimitive(const Ray& ray, const Primitive& primitive, Interaction& isect);

    static bool intersectSphere(const Ray& ray, const Sphere& sphere, Interaction& isect);
};