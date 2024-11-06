#pragma once

#include "../base/gl_utility.h"
#include "../base/transform.h"
#include "../base/bounding_box.h"

#include <string>
#include <vector>
#include <glm/glm.hpp>

class MeshletModel {
public:
    // Meshlet is a part of the model's mesh
    // Vertex i of the meshlet, i = 0, 1, ..., vertexCount - 1
    //          vertices[vertexIndices[vertexOffset + i]]
    // Triangle i of the meshlet, i = 0, 1, ..., primitiveCount - 1
    //       vertexOffset + primitiveIndices[primitiveOffset + 3 * i + 0]
    //       vertexOffset + primitiveIndices[primitiveOffset + 3 * i + 1]
    //       vertexOffset + primitiveIndices[primitiveOffset + 3 * i + 2]
    struct alignas(16) Meshlet {
        uint32_t vertexCount;
        uint32_t vertexOffset;
        uint32_t primitiveCount;
        uint32_t primitiveOffset;
    };

    struct alignas(16) Vertex {
        glm::vec3 position;
        float u;
        glm::vec3 normal;
        float v;

        bool operator==(const Vertex& rhs) const noexcept {
            return position == rhs.position && normal == rhs.normal && u == rhs.u && v == rhs.v;
        }
    };

    struct BV {
        alignas(16) glm::vec3 min;
        alignas(16) glm::vec3 max;
    };

public:
    MeshletModel(std::string const& path);

    MeshletModel(const MeshletModel& rhs) = default;

    MeshletModel(MeshletModel&& rhs) noexcept = default;

    ~MeshletModel() = default;

    MeshletModel& operator=(const MeshletModel& rhs) = default;
    
    MeshletModel& operator=(MeshletModel&& rhs) = default;

    std::vector<Vertex> const& getVertices() const noexcept {
        return m_vertices; 
    }

    std::vector<uint32_t> const& getVertexIndices() const noexcept {
        return m_vertexIndices;
    }

    std::vector<uint8_t> const& getPrimitiveIndices() const noexcept {
        return m_primitiveIndices;
    }

    std::vector<Meshlet> const& getMeshlets() const noexcept {
        return m_meshlets;
    }

    BoundingBox getAABB() const noexcept {
        return m_aabb;
    }

    std::vector<BV> const& getMeshletBVs() const noexcept {
        return m_meshletBVs;
    }

protected:
    std::vector<Vertex>   m_vertices;
    std::vector<uint32_t> m_vertexIndices;
    std::vector<uint8_t>  m_primitiveIndices;
    std::vector<Meshlet>  m_meshlets;
    std::vector<BV> m_meshletBVs;

    BoundingBox m_aabb;
};