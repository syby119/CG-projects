#pragma once

#include <string>
#include <vector>

#include "bounding_box.h"
#include "gl_utility.h"
#include "transform.h"
#include "vertex.h"

class Model {
public:
    Model(const std::string& filepath);

    Model(const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices);

    Model(Model&& rhs) noexcept;

    virtual ~Model();

    Model& operator=(const Model& rhs) = delete;

    Model& operator=(Model&& rhs) noexcept;

    GLuint getVao() const;

    GLuint getBoundingBoxVao() const;

    size_t getVertexCount() const;

    size_t getFaceCount() const;

    BoundingBox getBoundingBox() const;

    virtual void draw() const;

    virtual void drawBoundingBox() const;

    const std::vector<uint32_t>& getIndices() const {
        return m_indices;
    }
    const std::vector<Vertex>& getVertices() const {
        return m_vertices;
    }
    const Vertex& getVertex(int i) const {
        return m_vertices[i];
    }

public:
    Transform transform;

protected:
    // vertices of the table represented in model's own coordinate
    std::vector<Vertex> m_vertices;
    std::vector<uint32_t> m_indices;

    // bounding box
    BoundingBox m_boundingBox;

    // opengl objects
    GLuint m_vao = 0;
    GLuint m_vbo = 0;
    GLuint m_ebo = 0;

    GLuint m_boxVao = 0;
    GLuint m_boxVbo = 0;
    GLuint m_boxEbo = 0;

    void computeBoundingBox();

    void initGLResources();

    void initBoxGLResources();

    void cleanup();
};