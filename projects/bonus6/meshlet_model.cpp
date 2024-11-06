#include "meshlet_model.h"

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
    struct hash<MeshletModel::Vertex> {
        size_t operator()(const MeshletModel::Vertex& vertex) const noexcept {
            return ((hash<glm::vec3>()(vertex.position)
                ^ (hash<glm::vec3>()(vertex.normal) << 1)) >> 1)
                ^ (hash<glm::vec2>()(glm::vec2(vertex.u, vertex.v)) << 1);
        }
    };
}

MeshletModel::MeshletModel(std::string const& filepath) {
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;

    std::string warn, err;

    std::string::size_type index = filepath.find_last_of("/");
    std::string mtlBaseDir = filepath.substr(0, index + 1);

    if (!tinyobj::LoadObj(
        &attrib, &shapes, &materials, &warn, &err, filepath.c_str(), mtlBaseDir.c_str())) {
        throw std::runtime_error("load " + filepath + " failure: " + err);
    }

    if (!warn.empty()) {
        std::cerr << "Loading model " + filepath + " warnings: " << std::endl;
        std::cerr << warn << std::endl;
    }

    if (!err.empty()) {
        throw std::runtime_error("Loading model " + filepath + " error:\n" + err);
    }

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

            // check if the vertex appeared before to reduce redundant data
            if (uniqueVertices.count(vertex) == 0) {
                uniqueVertices[vertex] = static_cast<uint32_t>(m_vertices.size());
                m_vertices.push_back(vertex);
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

    std::vector<meshopt_Meshlet> meshlets;
    size_t const maxMeshlets{
        meshopt_buildMeshletsBound(indices.size(), maxVertexCount, maxPrimitiveCount)
    };

    meshlets.resize(maxMeshlets);
    m_vertexIndices.resize(maxMeshlets * maxVertexCount);
    m_primitiveIndices.resize(maxMeshlets * maxPrimitiveCount);

    size_t meshletCount = meshopt_buildMeshlets(
        meshlets.data(),                             // [O] array of meshopt_Meshlet
        m_vertexIndices.data(),                      // [O] array of uint32_t - meshlet to mesh index mappings
        m_primitiveIndices.data(),                   // [O] array of uint8_t - triangle indices
        indices.data(),                              // [I] pointer mesh vertex indices
        indices.size(),                              // [I] number of vertex indices
        reinterpret_cast<float*>(m_vertices.data()), // [I] pointer to vertex positions
        m_vertices.size(),                           // [I] number of vertex positions
        sizeof(Vertex),                              // [I] stride of vertex position elements
        maxVertexCount,                              // [I] maximum number of vertices per meshlet
        maxPrimitiveCount,                           // [I] maximum number of triangles per meshlet
        coneWeight);

    // make data more compact
    auto& last = meshlets[meshletCount - 1];
    m_vertexIndices.resize(last.vertex_offset + last.vertex_count);
    m_primitiveIndices.resize(last.triangle_offset + ((last.triangle_count * 3 + 3) & ~3));
    meshlets.resize(meshletCount);

    // adapt meshopt data to 
    for (size_t i = 0; i < meshlets.size(); ++i) {
        m_meshlets.push_back({
            meshlets[i].vertex_count,
            meshlets[i].vertex_offset,
            meshlets[i].triangle_count,
            meshlets[i].triangle_offset
            });
    }

    // generate AABBs for meshlets
    m_meshletBVs.reserve(m_meshlets.size());
    for (auto const& meshlet : m_meshlets) {
        BV aabb{
            glm::vec3(std::numeric_limits<float>::max()),
            -glm::vec3(std::numeric_limits<float>::max()),
        };

        for (size_t i = 0; i < meshlet.vertexCount; ++i) {
            auto const& position{ m_vertices[m_vertexIndices[meshlet.vertexOffset + i]].position };
            aabb.min = glm::min(aabb.min, position);
            aabb.max = glm::max(aabb.max, position);
        }

        m_meshletBVs.push_back(aabb);
    }

    // output statictis
    std::cout << filepath << std::endl;
    printf("Attribute               Count           Memory(Bytes)\n");
    printf("-----------------------------------------------------\n");
    printf("vertices           %10llu          %10llu \n",
        m_vertices.size(), m_vertices.size() * sizeof(Vertex));
    printf("indices            %10llu          %10llu \n",
        indices.size(), indices.size() * sizeof(uint32_t));
    printf("vertex indices     %10llu          %10llu \n",
        m_vertexIndices.size(), m_vertexIndices.size() * sizeof(uint32_t));
    printf("primitive indices  %10llu          %10llu \n",
        m_primitiveIndices.size(), m_primitiveIndices.size() * sizeof(uint8_t));
    printf("meshlets           %10llu          %10llu \n",
        m_meshlets.size(), m_meshlets.size() * sizeof(Meshlet));
    printf("meshletBVs         %10llu          %10llu \n",
        m_meshletBVs.size(), m_meshletBVs.size() * sizeof(BV));
    printf("-----------------------------------------------------\n");

    size_t meshletIndicesMemory{
        m_vertexIndices.size() * sizeof(uint32_t) +
        m_primitiveIndices.size() * sizeof(uint8_t) +
        m_meshlets.size() * sizeof(Meshlet) };
    size_t traditionalIndicesMemory{ indices.size() * sizeof(uint32_t) };
    printf("memory percentage -- meshlet / traditional: %f\n",
        1.0f * meshletIndicesMemory / traditionalIndicesMemory);
    printf("-----------------------------------------------------\n");
}