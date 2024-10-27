#include "model.h"

#include <algorithm>
#include <iostream>
#include <limits>
#include <unordered_map>

#include <meshoptimizer.h>
#include <tiny_obj_loader.h>


namespace experimental {
    Model::Model(std::string const& filepath) {
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


#if SEPERATE
        for (const auto& shape : shapes) {
            for (const auto& index : shape.mesh.indices) {
                m_positions.emplace_back(
                    attrib.vertices[3 * index.vertex_index + 0],
                    attrib.vertices[3 * index.vertex_index + 1],
                    attrib.vertices[3 * index.vertex_index + 2]
                );

                if (index.normal_index >= 0) {
                    m_normals.emplace_back(
                        attrib.normals[3 * index.normal_index + 0],
                        attrib.normals[3 * index.normal_index + 1],
                        attrib.normals[3 * index.normal_index + 2]
                    );
                }
                else {
                    // let's just add a zero normal
                    m_normals.emplace_back(0.0f, 0.0f, 0.0f);
                }

                if (index.texcoord_index >= 0) {
                    m_texCoords.emplace_back(
                        attrib.texcoords[2 * index.texcoord_index + 0],
                        attrib.texcoords[2 * index.texcoord_index + 1]
                    );
                }
                else {
                    // let's just add a zero uv
                    m_texCoords.emplace_back(0.0f, 0.0f);
                }

                m_indices.emplace_back(static_cast<uint32_t>(m_positions.size() - 1));
            }
        }
#else
        for (const auto& shape : shapes) {
            for (const auto& index : shape.mesh.indices) {
                Vertex vert;
                vert.position = glm::vec3(
                    attrib.vertices[3 * index.vertex_index + 0],
                    attrib.vertices[3 * index.vertex_index + 1],
                    attrib.vertices[3 * index.vertex_index + 2]
                );

                if (index.normal_index >= 0) {
                    vert.normal = glm::vec3(
                        attrib.normals[3 * index.normal_index + 0],
                        attrib.normals[3 * index.normal_index + 1],
                        attrib.normals[3 * index.normal_index + 2]
                    );
                }
                else {
                    // let's just add a zero normal
                    vert.normal = glm::vec3(0.0f, 0.0f, 0.0f);
                }

                if (index.texcoord_index >= 0) {
                    vert.u = attrib.texcoords[2 * index.texcoord_index + 0];
                    vert.v = attrib.texcoords[2 * index.texcoord_index + 1];
                }
                else {
                    // let's just add a zero uv
                    vert.u = 0.0f;
                    vert.v = 0.0f;
                }

                m_vertices.push_back(vert);
                m_indices.emplace_back(static_cast<uint32_t>(m_vertices.size() - 1));
            }
        }
#endif

#if DEBUG
#if SEPERATE
        m_positions.clear();
        m_normals.clear();
        m_texCoords.clear();
        m_indices.clear();

        m_positions.emplace_back(-1.f, -1.f, 0.f);
        m_normals.emplace_back(0.0f, 0.0f, 0.0f);
        m_texCoords.emplace_back(0.0f, 0.0f);

        m_positions.emplace_back(0.f, 1.f, 0.f);
        m_normals.emplace_back(0.0f, 0.0f, 0.0f);
        m_texCoords.emplace_back(0.0f, 0.0f);

        m_positions.emplace_back(1.f, -1.f, 0.f);
        m_normals.emplace_back(0.0f, 0.0f, 0.0f);
        m_texCoords.emplace_back(0.0f, 0.0f);

        m_indices.emplace_back(0);
        m_indices.emplace_back(1);
        m_indices.emplace_back(2);
#else
        m_vertices.resize(3);
        m_vertices[0].position = glm::vec3(-1.f, -1.f, 0.f);
        m_vertices[1].position = glm::vec3(0.f, 1.f, 0.f);
        m_vertices[2].position = glm::vec3(1.f, -1.f, 0.f);
        
        m_indices.clear();
        m_indices.emplace_back(0);
        m_indices.emplace_back(1);
        m_indices.emplace_back(2);
#endif
#endif
        // generate meshlets data
        constexpr size_t maxVertexCount = 64;
        constexpr size_t maxPrimitiveCount = 124;
        constexpr float coneWeight = 0.0f;

        std::vector<meshopt_Meshlet> meshlets;
        size_t const maxMeshlets{
            meshopt_buildMeshletsBound(m_indices.size(), maxVertexCount, maxPrimitiveCount)
        };

        meshlets.resize(maxMeshlets);
        m_vertexIndices.resize(maxMeshlets * maxVertexCount);
        m_primitiveIndices.resize(maxMeshlets * maxPrimitiveCount);

#if SEPERATE
        size_t meshletCount = meshopt_buildMeshlets(
            meshlets.data(),                              // [O] array of meshopt_Meshlet
            m_vertexIndices.data(),                       // [O] array of uint32_t - meshlet to mesh index mappings
            m_primitiveIndices.data(),                    // [O] array of uint8_t - triangle indices
            m_indices.data(),                             // [I] pointer mesh vertex indices
            m_indices.size(),                             // [I] number of vertex indices
            reinterpret_cast<float*>(m_positions.data()), // [I] pointer to vertex positions
            m_positions.size(),                           // [I] number of vertex positions
            sizeof(glm::vec3),                            // [I] stride of vertex position elements
            maxVertexCount,                               // [I] maximum number of vertices per meshlet
            maxPrimitiveCount,                            // [I] maximum number of triangles per meshlet
            coneWeight);                                  // [I] cone weight
#else
        size_t meshletCount = meshopt_buildMeshlets(
            meshlets.data(),                              // [O] array of meshopt_Meshlet
            m_vertexIndices.data(),                       // [O] array of uint32_t - meshlet to mesh index mappings
            m_primitiveIndices.data(),                    // [O] array of uint8_t - triangle indices
            m_indices.data(),                             // [I] pointer mesh vertex indices
            m_indices.size(),                             // [I] number of vertex indices
            reinterpret_cast<float*>(m_vertices.data()), // [I] pointer to vertex positions
            m_vertices.size(),                           // [I] number of vertex positions
            sizeof(Vertex),                            // [I] stride of vertex position elements
            maxVertexCount,                               // [I] maximum number of vertices per meshlet
            maxPrimitiveCount,                            // [I] maximum number of triangles per meshlet
            coneWeight);
#endif

        // make data more compact
        auto& last = meshlets[meshletCount - 1];
        m_vertexIndices.resize(last.vertex_offset + last.vertex_count);
        m_primitiveIndices.resize(last.triangle_offset + ((last.triangle_count * 3 + 3) & ~3));
        meshlets.resize(meshletCount);

        for (size_t i = 0; i < meshlets.size(); ++i) {
            m_meshlets.push_back({
                meshlets[i].vertex_count,
                meshlets[i].vertex_offset,
                meshlets[i].triangle_count,
                meshlets[i].triangle_offset
                });
        }

        // generate gpu data
#if SEPERATE
        //printf("m_positions: %p\n", m_positions.data());

        // + position
        glGenBuffers(1, &m_positionBuffer);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_positionBuffer);
        glBufferData(GL_SHADER_STORAGE_BUFFER,
            m_positions.size() * sizeof(glm::vec3), m_positions.data(), GL_STATIC_DRAW);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

        // + normal
        glGenBuffers(1, &m_normalBuffer);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_normalBuffer);
        glBufferData(GL_SHADER_STORAGE_BUFFER,
            m_normals.size() * sizeof(glm::vec3), m_normals.data(), GL_STATIC_DRAW);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

        // + texCoord
        glGenBuffers(1, &m_texCoordBuffer);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_texCoordBuffer);
        glBufferData(GL_SHADER_STORAGE_BUFFER,
            m_texCoords.size() * sizeof(glm::vec2), m_texCoords.data(), GL_STATIC_DRAW);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
#else
        glGenBuffers(1, &m_vertexBuffer);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_vertexBuffer);
        glBufferData(GL_SHADER_STORAGE_BUFFER,
            m_vertices.size() * sizeof(Vertex), m_vertices.data(), GL_STATIC_DRAW);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
#endif

        // + vertex indices
        glGenBuffers(1, &m_vertexIndicesBuffer);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_vertexIndicesBuffer);
        glBufferData(GL_SHADER_STORAGE_BUFFER,
            m_vertexIndices.size() * sizeof(uint32_t), m_vertexIndices.data(), GL_STATIC_DRAW);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

        // + primitive indices
        // we encode the 3 uint8_t indices into a single uint32_t for better performance
        glGenBuffers(1, &m_primitiveIndicesBuffer);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_primitiveIndicesBuffer);
        glBufferData(GL_SHADER_STORAGE_BUFFER,
            m_primitiveIndices.size() * sizeof(uint8_t), m_primitiveIndices.data(), GL_STATIC_DRAW);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

        // + meshlet
        glGenBuffers(1, &m_meshletBuffer);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_meshletBuffer);
        glBufferData(GL_SHADER_STORAGE_BUFFER,
            m_meshlets.size() * sizeof(Meshlet), m_meshlets.data(), GL_STATIC_DRAW);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    }

    //Model::Model(Model&& rhs) noexcept
    //    : m_positions{ std::move(rhs.m_positions) }
    //    , m_normals{ std::move(rhs.m_normals) }
    //    , m_texCoords{ std::move(rhs.m_texCoords) }
    //    , m_indices{ std::move(rhs.m_indices) }
    //    , m_vertexIndices{ std::move(rhs.m_vertexIndices) }
    //    , m_primitiveIndices{ std::move(rhs.m_primitiveIndices) }
    //    , m_positionBuffer{ std::move(rhs.m_positionBuffer) }
    //    , m_normalBuffer{ std::move(rhs.m_normalBuffer) }
    //    , m_texCoordBuffer{ std::move(rhs.m_texCoordBuffer) }
    //    , m_vertexIndicesBuffer{ std::move(rhs.m_vertexIndicesBuffer) }
    //    , m_primitiveIndicesBuffer{ std::move(rhs.m_primitiveIndicesBuffer) }
    //    , m_meshletBuffer{ std::move(rhs.m_meshletBuffer) } {
    //    rhs.m_positionBuffer = 0;
    //    rhs.m_normalBuffer = 0;
    //    rhs.m_texCoordBuffer = 0;
    //    rhs.m_vertexIndicesBuffer = 0;
    //    rhs.m_primitiveIndicesBuffer = 0;
    //    rhs.m_meshletBuffer = 0;
    //}

    Model::~Model() {
#if SEPERATE
        if (m_positionBuffer) {
            glDeleteBuffers(1, &m_positionBuffer);
        }

        if (m_normalBuffer) {
            glDeleteBuffers(1, &m_normalBuffer);
        }

        if (m_texCoordBuffer) {
            glDeleteBuffers(1, &m_texCoordBuffer);
        }
#else
        if (m_vertexBuffer) {
            glDeleteBuffers(1, &m_vertexBuffer);
        }
#endif
        if (m_vertexIndicesBuffer) {
            glDeleteBuffers(1, &m_vertexIndicesBuffer);
        }

        if (m_primitiveIndicesBuffer) {
            glDeleteBuffers(1, &m_primitiveIndicesBuffer);
        }

        if (m_meshletBuffer) {
            glDeleteBuffers(1, &m_meshletBuffer);
        }
    }

    void Model::draw() const {
#if SEPERATE
        constexpr uint32_t positionBinding{ 0 };
        constexpr uint32_t normalBinding{ 1 };
        constexpr uint32_t texCoordBinding{ 2 };
#else
        constexpr uint32_t m_vertexBufferBinding{ 0 };
#endif
        constexpr uint32_t vertexIndicesBinding{ 3 };
        constexpr uint32_t primitiveIndicesBinding{ 4 };
        constexpr uint32_t meshletBinding{ 5 };

#if SEPERATE
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_positionBuffer);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, positionBinding, m_positionBuffer);

        glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_normalBuffer);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, normalBinding, m_normalBuffer);

        glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_texCoordBuffer);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, texCoordBinding, m_texCoordBuffer);
#else
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_vertexBuffer);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, m_vertexBufferBinding, m_vertexBuffer);
#endif

        glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_vertexIndicesBuffer);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, vertexIndicesBinding, m_vertexIndicesBuffer);

        glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_primitiveIndicesBuffer);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, primitiveIndicesBinding, m_primitiveIndicesBuffer);

        glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_meshletBuffer);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, meshletBinding, m_meshletBuffer);

        glDrawMeshTasksNV(0, static_cast<uint32_t>(m_meshlets.size()));

        glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    }
}