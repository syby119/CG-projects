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
        // TODO: recursive build BVH 
        return node;
    }
}

int BVH::toLinearTree(BVHBuildNode* root, int* offset) {
    if (root == nullptr) {
        return -1;
    }

    int nodeIdx = *offset;
    *offset += 1;
    int leftIdx = toLinearTree(root->leftChild, offset);
    int rightIdx = toLinearTree(root->rightChild, offset);
    nodes[nodeIdx].box = root->bound;
    if (leftIdx == -1 && rightIdx == -1) {
        nodes[nodeIdx].type = BVHNode::Type::Leaf;
        nodes[nodeIdx].startIndex = root->startIdx;
        nodes[nodeIdx].nPrimitives = root->nPrimitives;
    } else {
        nodes[nodeIdx].type = BVHNode::Type::NonLeaf;
        nodes[nodeIdx].leftChild = leftIdx;
        nodes[nodeIdx].rightChild = rightIdx;
    }

    return nodeIdx;
}

bool BVH::intersect(const Ray& ray, Interaction& isect) {
    bool hit = false;
    glm::vec3 invDir = glm::vec3(1.0f / ray.dir.x, 1.0f / ray.dir.y, 1.0f / ray.dir.z);
    int isDirNeg[3];
    isDirNeg[0] = ray.dir.x < 0 ? 1 : 0;
    isDirNeg[1] = ray.dir.y < 0 ? 1 : 0;
    isDirNeg[2] = ray.dir.z < 0 ? 1 : 0;
    int currentNodeIndex = 0;
    int toVisitOffset = 0;
    int nodesToVisit[128];
    BVHNode node;
    while (true) {
        node = nodes[currentNodeIndex];
        if (node.box.intersect(ray, invDir, isDirNeg)) {
            if (node.type == BVHNode::Type::Leaf) {
                int firstIndex = node.startIndex;
                int nPrimitives = node.nPrimitives;

                Primitive primitive;
                for (int i = 0; i < nPrimitives; ++i) {
                    primitive = orderedPrimitives[firstIndex + i];
                    if (intersectPrimitive(ray, primitive, isect)) {
                        hit = true;
                    }
                }

                if (toVisitOffset == 0) {
                    break;
                }

                currentNodeIndex = nodesToVisit[--toVisitOffset];
            } else {
                int leftChild = node.leftChild;
                int rightChild = node.rightChild;
                nodesToVisit[toVisitOffset++] = rightChild;
                currentNodeIndex = leftChild;
            }
        } else {
            if (toVisitOffset == 0) {
                break;
            }
            currentNodeIndex = nodesToVisit[--toVisitOffset];
        }
    }

    return hit;
}

AABB BVH::getAABB(const Primitive& prim) {
    if (prim.type == Primitive::Type::Sphere) {
        return getSphereAABB(*prim.sphere);
    }
    else {
        return getTriangleAABB(*prim.triangle);
    }
}

bool BVH::intersectPrimitive(const Ray& ray, const Primitive& primitive, Interaction& isect) {
    if (intersectSphere(ray, *primitive.sphere, isect)) {
        isect.primitive = primitive;
        return true;
    }

    return false;
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

bool BVH::intersectSphere(const Ray& ray, const Sphere& sphere, Interaction& isect) {
    float a = glm::dot(ray.dir, ray.dir);
    float b = glm::dot(ray.dir, ray.o - sphere.position);
    float c = glm::dot(ray.o - sphere.position, ray.o - sphere.position) - sphere.radius * sphere.radius;
    float discriminant = b * b - a * c;

    if (discriminant >= 0) {
        float t1 = (-b - std::sqrt(discriminant)) / a;
        float t2 = (-b + std::sqrt(discriminant)) / a;

        if ((1e-3f <= t1 && t1 < ray.tMax) || (1e-3f <= t2 && t2 < ray.tMax)) {
            float t = (1e-3f <= t1 && t1 < ray.tMax) ? t1 : t2;
            ray.tMax = t;
            isect.hitPoint.position = ray.o + t * ray.dir;
            isect.hitPoint.normal = glm::normalize(isect.hitPoint.position - sphere.position);
            return true;
        }
    }

    return false;
}