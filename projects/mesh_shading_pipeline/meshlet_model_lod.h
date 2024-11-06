#pragma once

#include "../base/glsl_program.h"
#include "../base/bounding_box.h"

class MeshletModelLod {
public:
    struct Meshlet {
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

    struct LodInfo {
        uint32_t meshletOffset;
        uint32_t meshletCount;
    };

public:
    MeshletModelLod(const std::vector<std::string>& paths);

    MeshletModelLod(const MeshletModelLod& rhs) noexcept = default;

    MeshletModelLod(MeshletModelLod&& rhs) noexcept = default;

    ~MeshletModelLod() = default;

    MeshletModelLod& operator=(const MeshletModelLod& rhs) = default;

    MeshletModelLod& operator=(MeshletModelLod&& rhs) = default;

    std::vector<Vertex> const& getVertices() const noexcept { 
        return m_combinedVertices; 
    }

    std::vector<uint32_t> const& getVertexIndices() const noexcept {
        return m_combinedVertexIndices; 
    }

    std::vector<uint8_t> const& getPrimitiveIndices() const noexcept { 
        return m_combinedPrimitiveIndices; 
    }

    std::vector<Meshlet> const& getMeshlets() const noexcept { 
        return m_combinedMeshlets; 
    }

    std::vector<LodInfo> const& getMeshletLodInfos() const noexcept { 
        return m_meshletLodInfos;
    }

    uint32_t getLodCount() const noexcept {
        return m_meshletLodInfos.size();
    }

    BoundingBox getBoundingBox() const noexcept { 
        return m_aabb; 
    }

    glm::vec3 getCenter() const noexcept {
        return 0.5f * (m_aabb.min + m_aabb.max);
    }

    std::vector<BV> const& getMeshletBVs() const noexcept { 
        return m_combinedMeshletBVs; 
    }

private:
    std::vector<Vertex> m_combinedVertices;
    std::vector<uint32_t> m_combinedVertexIndices;
    std::vector<uint8_t> m_combinedPrimitiveIndices;
    std::vector<Meshlet> m_combinedMeshlets;
    std::vector<BV> m_combinedMeshletBVs;
    std::vector<LodInfo> m_meshletLodInfos;
    
    BoundingBox m_aabb;
};