#include <iostream>
#include "bvh.h"

void BVH::constructBVH(std::vector<Primitive>& primitives) {
    std::vector<PrimitiveInfo> primInfo;
    for (int i = 0; i < primitives.size(); ++i) {
        primInfo.push_back({ i, getAABB(primitives[i]) });
    }

    int totalNode = 0;
    BVHBuildNode* root = recursiveBuild(
        primitives, primInfo, 0, static_cast<int>(primInfo.size()), &totalNode);

    nodes.resize(totalNode);
    int offset = 0;
    toLinearTree(root, &offset);
}

BVHBuildNode* BVH::recursiveBuild(
    const std::vector<Primitive>& primitives,
    std::vector<PrimitiveInfo>& primInfo, 
    int start, int end, int* totalNodes
) {
    BVHBuildNode* node = new BVHBuildNode;
    *totalNodes += 1;
    int nPrimitives = end - start;
    if (nPrimitives == 1) {
        AABB box;
        int startId = static_cast<int>(orderedPrimitives.size());
        for (int i = start; i < end; ++i) {
            box = unionAABB(box, primInfo[i].box);
            orderedPrimitives.push_back(primitives[primInfo[i].pid]);
        }
        node->initLeafNode(box, startId, nPrimitives);
        return node;
    } else {
        // TODO : recursive build BVH 
        return node;
    }
}

int BVH::toLinearTree(BVHBuildNode* root, int* offset) {
    return 0;
}

AABB BVH::getAABB(const Primitive& prim) {
    if (prim.type == Primitive::Type::Sphere) {
        return getSphereAABB(*prim.sphere);
    }
    else {
        return getTriangleAABB(*prim.triangle);
    }
}

AABB BVH::getTriangleAABB(const Triangle& triangle) {
    const auto& p1 = triangle.vertices[triangle.v[0]].position;
    const auto& p2 = triangle.vertices[triangle.v[1]].position;
    const auto& p3 = triangle.vertices[triangle.v[2]].position;
    return unionAABB(AABB(p1, p2), p3);
}

AABB BVH::getSphereAABB(const Sphere& sphere) {
    return AABB(sphere.position - glm::vec3(sphere.radius),
        sphere.position + glm::vec3(sphere.radius));
}