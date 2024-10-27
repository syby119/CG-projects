#pragma once

#include "../base/gl_utility.h"
#include "../base/transform.h"

#include <string>
#include <vector>
#include <glm/glm.hpp>

#define SEPERATE 1
#define DEBUG    1

namespace experimental {
    class Model {
    public:
        // Meshlet is a part of the model's mesh
        // Vertex i of the meshlet, i = 0, 1, ..., vertexCount - 1
        //          vertices[vertexIndices[vertexOffset + i]]
        // Triangle i of the meshlet, i = 0, 1, ..., primitiveCount - 1
        //       primitiveIndices[primitiveOffset + 3 * i + 0]
        //       primitiveIndices[primitiveOffset + 3 * i + 1]
        //       primitiveIndices[primitiveOffset + 3 * i + 2]
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
        };

    public:
        Model(std::string const& path);

        Model(Model&& rhs) noexcept;

        ~Model();

        void draw() const;

    public:
        Transform transform;

    private:

#if SEPERATE
        std::vector<glm::vec3> m_positions;
        std::vector<glm::vec3> m_normals;
        std::vector<glm::vec2> m_texCoords;
#else
        std::vector<Vertex> m_vertices;
#endif

        std::vector<uint32_t> m_indices;

        std::vector<uint32_t>  m_vertexIndices;
        std::vector<uint8_t>   m_primitiveIndices;
        std::vector<Meshlet>   m_meshlets;

        // GPU resources, each is a shader storage buffer
#if SEPERATE
        GLuint m_positionBuffer{ 0 };
        GLuint m_normalBuffer{ 0 };
        GLuint m_texCoordBuffer{ 0 };
#else
        GLuint m_vertexBuffer{ 0 };
#endif
        GLuint m_vertexIndicesBuffer{ 0 };
        GLuint m_primitiveIndicesBuffer{ 0 };
        GLuint m_meshletBuffer{ 0 };
    };
}