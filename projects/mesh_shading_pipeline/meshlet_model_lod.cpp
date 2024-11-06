#include "meshlet_model_lod.h"

#include <algorithm>
#include <iostream>
#include <limits>
#include <unordered_map>

#include <meshoptimizer.h>
#include <tiny_obj_loader.h>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>

namespace std {
    template <>
    struct hash<MeshletModelLod::Vertex> {
        size_t operator()(const MeshletModelLod::Vertex& vertex) const noexcept {
            return ((hash<glm::vec3>()(vertex.position)
                ^ (hash<glm::vec3>()(vertex.normal) << 1)) >> 1)
                ^ (hash<glm::vec2>()(glm::vec2(vertex.u, vertex.v)) << 1);
        }
    };
}

MeshletModelLod::MeshletModelLod(const std::vector<std::string>& filepaths) {
    for (size_t i = 0; i < filepaths.size(); ++i) {
        // read obj file
        tinyobj::attrib_t attrib;
        std::vector<tinyobj::shape_t> shapes;
        std::vector<tinyobj::material_t> materials;

        std::string warn, err;

        std::string::size_type index = filepaths[i].find_last_of("/");
        std::string mtlBaseDir = filepaths[i].substr(0, index + 1);

        if (!tinyobj::LoadObj(
            &attrib, &shapes, &materials, &warn, &err, filepaths[i].c_str(), mtlBaseDir.c_str())) {
            throw std::runtime_error("load " + filepaths[i] + " failure: " + err);
        }

        if (!warn.empty()) {
            std::cerr << "Loading model " + filepaths[i] + " warnings: " << std::endl;
            std::cerr << warn << std::endl;
        }

        if (!err.empty()) {
            throw std::runtime_error("Loading model " + filepaths[i] + " error:\n" + err);
        }

        // parse data
        std::vector<Vertex> vertices;
        std::vector<uint32_t> indices;
        std::unordered_map<Vertex, uint32_t> uniqueVertices;

        for (const auto& shape : shapes) {
            for (const auto& index : shape.mesh.indices) {
                Vertex vertex{};
                vertex.position = glm::vec3(
                    attrib.vertices[3 * index.vertex_index + 0],
                    attrib.vertices[3 * index.vertex_index + 1],
                    attrib.vertices[3 * index.vertex_index + 2]
                );

                if (index.normal_index >= 0) {
                    vertex.normal = glm::vec3(
                        attrib.normals[3 * index.normal_index + 0],
                        attrib.normals[3 * index.normal_index + 1],
                        attrib.normals[3 * index.normal_index + 2]
                    );
                }

                if (index.texcoord_index >= 0) {
                    vertex.u = attrib.texcoords[2 * index.texcoord_index + 0];
                    vertex.v = attrib.texcoords[2 * index.texcoord_index + 1];
                }

                if (uniqueVertices.count(vertex) == 0) {
                    uniqueVertices[vertex] = static_cast<uint32_t>(vertices.size());
                    vertices.push_back(vertex);
                    m_aabb.min = glm::min(vertex.position, m_aabb.min);
                    m_aabb.max = glm::max(vertex.position, m_aabb.max);
                }

                indices.push_back(uniqueVertices[vertex]);
            }
        }

        // generate meshlets data
        constexpr size_t maxVertexCount = 64;
        constexpr size_t maxPrimitiveCount = 124;
        constexpr float coneWeight = 0.0f;

        std::vector<uint32_t> vertexIndices;
        std::vector<uint8_t> primitiveIndices;
        std::vector<meshopt_Meshlet> meshlets;
        size_t const maxMeshlets{
            meshopt_buildMeshletsBound(indices.size(), maxVertexCount, maxPrimitiveCount)
        };

        meshlets.resize(maxMeshlets);
        vertexIndices.resize(maxMeshlets * maxVertexCount);
        primitiveIndices.resize(maxMeshlets * maxPrimitiveCount);

        size_t meshletCount = meshopt_buildMeshlets(
            meshlets.data(),                            // [O] array of meshopt_Meshlet
            vertexIndices.data(),                       // [O] array of uint32_t - meshlet to mesh index mappings
            primitiveIndices.data(),                    // [O] array of uint8_t - triangle indices
            indices.data(),                             // [I] pointer mesh vertex indices
            indices.size(),                             // [I] number of vertex indices
            reinterpret_cast<float*>(vertices.data()),  // [I] pointer to vertex positions
            vertices.size(),                            // [I] number of vertex positions
            sizeof(Vertex),                             // [I] stride of vertex position elements
            maxVertexCount,                             // [I] maximum number of vertices per meshlet
            maxPrimitiveCount,                          // [I] maximum number of triangles per meshlet
            coneWeight);

        // make data more compact
        auto& last = meshlets[meshletCount - 1];
        vertexIndices.resize(last.vertex_offset + last.vertex_count);
        primitiveIndices.resize(last.triangle_offset + ((last.triangle_count * 3 + 3) & ~3));
        meshlets.resize(meshletCount);

        // combine data
        m_meshletLodInfos.push_back({
            static_cast<uint32_t>(m_combinedMeshlets.size()),
            static_cast<uint32_t>(meshlets.size())
        });

        const uint32_t vertexOffset{ static_cast<uint32_t>(m_combinedVertices.size()) };
        const uint32_t vertexIndicesOffset{ static_cast<uint32_t>(m_combinedVertexIndices.size()) };
        const uint32_t primitiveIndicesOffset{ static_cast<uint32_t>(m_combinedPrimitiveIndices.size()) };

        std::copy(vertices.begin(), vertices.end(), std::back_inserter(m_combinedVertices));

        for (auto vertexIndex : vertexIndices) {
            m_combinedVertexIndices.push_back(vertexIndex + vertexOffset);
        }

        for (size_t i = 0; i < meshlets.size(); ++i) {
            m_combinedMeshlets.push_back({
                meshlets[i].vertex_count,
                meshlets[i].vertex_offset + vertexIndicesOffset,
                meshlets[i].triangle_count,
                meshlets[i].triangle_offset + primitiveIndicesOffset
                });
        }

        std::copy(primitiveIndices.begin(), primitiveIndices.end(),
            std::back_inserter(m_combinedPrimitiveIndices));
    }

    // generate AABBs for meshlets
    m_combinedMeshletBVs.reserve(m_combinedMeshlets.size());
    for (auto const& meshlet : m_combinedMeshlets) {
        BV aabb{
            glm::vec3(std::numeric_limits<float>::max()),
            -glm::vec3(std::numeric_limits<float>::max()),
        };

        for (size_t i = 0; i < meshlet.vertexCount; ++i) {
            auto const& position{
                m_combinedVertices[m_combinedVertexIndices[meshlet.vertexOffset + i]].position };
            aabb.min = glm::min(aabb.min, position);
            aabb.max = glm::max(aabb.max, position);
        }

        m_combinedMeshletBVs.push_back(aabb);
    }
}
