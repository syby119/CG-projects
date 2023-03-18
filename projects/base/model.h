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

    GLuint getVao() const;

    GLuint getBoundingBoxVao() const;

    size_t getVertexCount() const;

    size_t getFaceCount() const;

    BoundingBox getBoundingBox() const;

    virtual void draw() const;

    virtual void drawBoundingBox() const;

    const std::vector<uint32_t>& getIndices() const {
        return _indices;
    }
    const std::vector<Vertex>& getVertices() const {
        return _vertices;
    }
    const Vertex& getVertex(int i) const {
        return _vertices[i];
    }

public:
    Transform transform;

protected:
    // vertices of the table represented in model's own coordinate
    std::vector<Vertex> _vertices;
    std::vector<uint32_t> _indices;

    // bounding box
    BoundingBox _boundingBox;

    // opengl objects
    GLuint _vao = 0;
    GLuint _vbo = 0;
    GLuint _ebo = 0;

    GLuint _boxVao = 0;
    GLuint _boxVbo = 0;
    GLuint _boxEbo = 0;

    void computeBoundingBox();

    void initGLResources();

    void initBoxGLResources();

    void cleanup();
};