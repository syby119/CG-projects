#include <algorithm>
#include <iostream>
#include <limits>
#include <unordered_map>

#ifdef _MSC_VER
    #pragma warning(push)
    #pragma warning(disable : 4819)
#endif

#include <tiny_obj_loader.h>

#ifdef _MSC_VER
    #pragma warning(pop)
#endif

#include "model.h"

Model::Model(const std::string& filepath) {
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

    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
    std::unordered_map<Vertex, uint32_t> uniqueVertices;

    for (const auto& shape : shapes) {
        for (const auto& index : shape.mesh.indices) {
            Vertex vertex{};

            vertex.position.x = attrib.vertices[3 * index.vertex_index + 0];
            vertex.position.y = attrib.vertices[3 * index.vertex_index + 1];
            vertex.position.z = attrib.vertices[3 * index.vertex_index + 2];

            if (index.normal_index >= 0) {
                vertex.normal.x = attrib.normals[3 * index.normal_index + 0];
                vertex.normal.y = attrib.normals[3 * index.normal_index + 1];
                vertex.normal.z = attrib.normals[3 * index.normal_index + 2];
            }

            if (index.texcoord_index >= 0) {
                vertex.texCoord.x = attrib.texcoords[2 * index.texcoord_index + 0];
                vertex.texCoord.y = attrib.texcoords[2 * index.texcoord_index + 1];
            }

            // check if the vertex appeared before to reduce redundant data
            if (uniqueVertices.count(vertex) == 0) {
                uniqueVertices[vertex] = static_cast<uint32_t>(vertices.size());
                vertices.push_back(vertex);
            }

            indices.push_back(uniqueVertices[vertex]);
        }
    }

    m_vertices = vertices;
    m_indices = indices;

    computeBoundingBox();

    initGLResources();

    initBoxGLResources();

    GLenum error = glGetError();
    if (error != GL_NO_ERROR) {
        cleanup();
        throw std::runtime_error("OpenGL Error: " + std::to_string(error));
    }
}

Model::Model(const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices)
    : m_vertices(vertices), m_indices(indices) {

    computeBoundingBox();

    initGLResources();

    initBoxGLResources();

    GLenum error = glGetError();
    if (error != GL_NO_ERROR) {
        cleanup();
        throw std::runtime_error("OpenGL Error: " + std::to_string(error));
    }
}

Model::Model(Model&& rhs) noexcept
    : m_vertices(std::move(rhs.m_vertices)), m_indices(std::move(rhs.m_indices)),
      m_boundingBox(std::move(rhs.m_boundingBox)), m_vao(rhs.m_vao), m_vbo(rhs.m_vbo),
      m_ebo(rhs.m_ebo), m_boxVao(rhs.m_boxVao), m_boxVbo(rhs.m_boxVbo), m_boxEbo(rhs.m_boxEbo) {
    m_vao = 0;
    m_vbo = 0;
    m_ebo = 0;
    m_boxVao = 0;
    m_boxVbo = 0;
    m_boxEbo = 0;
}

Model::~Model() {
    cleanup();
}

Model& Model::operator=(Model&& rhs) noexcept {
    if (this != &rhs) {
        m_vertices = std::move(rhs.m_vertices);
        m_indices = std::move(rhs.m_indices);
        std::swap(m_vao, rhs.m_vao);
        std::swap(m_vbo, rhs.m_vbo);
        std::swap(m_ebo, rhs.m_ebo);
        std::swap(m_boxVao, rhs.m_boxVao);
        std::swap(m_boxVbo, rhs.m_boxVbo);
        std::swap(m_boxEbo, rhs.m_boxEbo);
    }

    return *this;
}

BoundingBox Model::getBoundingBox() const {
    return m_boundingBox;
}

void Model::draw() const {
    glBindVertexArray(m_vao);
    glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(m_indices.size()), GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}

void Model::drawBoundingBox() const {
    glBindVertexArray(m_boxVao);
    glDrawElements(GL_LINES, 24, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}

GLuint Model::getVao() const {
    return m_vao;
}

GLuint Model::getBoundingBoxVao() const {
    return m_boxVao;
}

size_t Model::getVertexCount() const {
    return m_vertices.size();
}

size_t Model::getFaceCount() const {
    return m_indices.size() / 3;
}

void Model::initGLResources() {
    // create a vertex array object
    glGenVertexArrays(1, &m_vao);
    // create a vertex buffer object
    glGenBuffers(1, &m_vbo);
    // create a element array buffer
    glGenBuffers(1, &m_ebo);

    glBindVertexArray(m_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(
        GL_ARRAY_BUFFER, sizeof(Vertex) * m_vertices.size(), m_vertices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ebo);
    glBufferData(
        GL_ELEMENT_ARRAY_BUFFER, m_indices.size() * sizeof(uint32_t), m_indices.data(),
        GL_STATIC_DRAW);

    // specify layout, size of a vertex, data type, normalize, sizeof vertex array, offset of the
    // attribute
    glVertexAttribPointer(
        0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, position));
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(
        1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(
        2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, texCoord));
    glEnableVertexAttribArray(2);

    glBindVertexArray(0);
}

void Model::computeBoundingBox() {
    float minX = std::numeric_limits<float>::max();
    float minY = std::numeric_limits<float>::max();
    float minZ = std::numeric_limits<float>::max();
    float maxX = -std::numeric_limits<float>::max();
    float maxY = -std::numeric_limits<float>::max();
    float maxZ = -std::numeric_limits<float>::max();

    for (const auto& v : m_vertices) {
        minX = std::min(v.position.x, minX);
        minY = std::min(v.position.y, minY);
        minZ = std::min(v.position.z, minZ);
        maxX = std::max(v.position.x, maxX);
        maxY = std::max(v.position.y, maxY);
        maxZ = std::max(v.position.z, maxZ);
    }

    m_boundingBox.min = glm::vec3(minX, minY, minZ);
    m_boundingBox.max = glm::vec3(maxX, maxY, maxZ);
}

void Model::initBoxGLResources() {
    std::vector<glm::vec3> boxVertices = {
        glm::vec3(m_boundingBox.min.x, m_boundingBox.min.y, m_boundingBox.min.z),
        glm::vec3(m_boundingBox.max.x, m_boundingBox.min.y, m_boundingBox.min.z),
        glm::vec3(m_boundingBox.min.x, m_boundingBox.max.y, m_boundingBox.min.z),
        glm::vec3(m_boundingBox.max.x, m_boundingBox.max.y, m_boundingBox.min.z),
        glm::vec3(m_boundingBox.min.x, m_boundingBox.min.y, m_boundingBox.max.z),
        glm::vec3(m_boundingBox.max.x, m_boundingBox.min.y, m_boundingBox.max.z),
        glm::vec3(m_boundingBox.min.x, m_boundingBox.max.y, m_boundingBox.max.z),
        glm::vec3(m_boundingBox.max.x, m_boundingBox.max.y, m_boundingBox.max.z),
    };

    std::vector<uint32_t> boxIndices = {0, 1, 0, 2, 0, 4, 3, 1, 3, 2, 3, 7,
                                        5, 4, 5, 1, 5, 7, 6, 4, 6, 7, 6, 2};

    glGenVertexArrays(1, &m_boxVao);
    glGenBuffers(1, &m_boxVbo);
    glGenBuffers(1, &m_boxEbo);

    glBindVertexArray(m_boxVao);
    glBindBuffer(GL_ARRAY_BUFFER, m_boxVbo);
    glBufferData(
        GL_ARRAY_BUFFER, boxVertices.size() * sizeof(glm::vec3), boxVertices.data(),
        GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_boxEbo);
    glBufferData(
        GL_ELEMENT_ARRAY_BUFFER, boxIndices.size() * sizeof(uint32_t), boxIndices.data(),
        GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), 0);
    glEnableVertexAttribArray(0);

    glBindVertexArray(0);
}

void Model::cleanup() {
    if (m_boxEbo) {
        glDeleteBuffers(1, &m_boxEbo);
        m_boxEbo = 0;
    }

    if (m_boxVbo) {
        glDeleteBuffers(1, &m_boxVbo);
        m_boxVbo = 0;
    }

    if (m_boxVao) {
        glDeleteVertexArrays(1, &m_boxVao);
        m_boxVao = 0;
    }

    if (m_ebo != 0) {
        glDeleteBuffers(1, &m_ebo);
        m_ebo = 0;
    }

    if (m_vbo != 0) {
        glDeleteBuffers(1, &m_vbo);
        m_vbo = 0;
    }

    if (m_vao != 0) {
        glDeleteVertexArrays(1, &m_vao);
        m_vao = 0;
    }
}