#include "instanced_model.h"
#include <iostream>

InstancedModel::InstancedModel(
    const std::string& filepath, const std::vector<glm::mat4>& modelMatrices)
    : Model(filepath), _modelMatrices(modelMatrices) {
    glBindVertexArray(_vao);

    glGenBuffers(1, &_instanceVbo);
    glBindBuffer(GL_ARRAY_BUFFER, _instanceVbo);
    glBufferData(
        GL_ARRAY_BUFFER, _modelMatrices.size() * sizeof(glm::mat4), _modelMatrices.data(),
        GL_STATIC_DRAW);

    constexpr GLsizei stride = sizeof(glm::mat4);
    constexpr GLsizei unitSize = sizeof(glm::vec4);
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, stride, (void*)(0 * unitSize));
    glEnableVertexAttribArray(4);
    glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, stride, (void*)(1 * unitSize));
    glEnableVertexAttribArray(5);
    glVertexAttribPointer(5, 4, GL_FLOAT, GL_FALSE, stride, (void*)(2 * unitSize));
    glEnableVertexAttribArray(6);
    glVertexAttribPointer(6, 4, GL_FLOAT, GL_FALSE, stride, (void*)(3 * unitSize));

    glVertexAttribDivisor(3, 1);
    glVertexAttribDivisor(4, 1);
    glVertexAttribDivisor(5, 1);
    glVertexAttribDivisor(6, 1);

    glBindVertexArray(0);

    glBindVertexArray(_boxVao);
    glBindBuffer(GL_ARRAY_BUFFER, _instanceVbo);

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, stride, (void*)(0 * unitSize));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, stride, (void*)(1 * unitSize));
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, stride, (void*)(2 * unitSize));
    glEnableVertexAttribArray(4);
    glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, stride, (void*)(3 * unitSize));

    glVertexAttribDivisor(1, 1);
    glVertexAttribDivisor(2, 1);
    glVertexAttribDivisor(3, 1);
    glVertexAttribDivisor(4, 1);

    glBindVertexArray(0);
}

InstancedModel::InstancedModel(InstancedModel&& rhs) noexcept
    : Model(std::move(rhs)), _instanceVbo(rhs._instanceVbo) {
    rhs._instanceVbo = 0;
}

InstancedModel::~InstancedModel() {
    if (_instanceVbo) {
        glDeleteBuffers(1, &_instanceVbo);
        _instanceVbo = 0;
    }
}

int InstancedModel::getInstanceCount() const {
    return static_cast<int>(_modelMatrices.size());
}

glm::mat4 InstancedModel::getModelMatrix(int index) const {
    return _modelMatrices[index];
}

const std::vector<glm::mat4>& InstancedModel::getModelMatrices() const {
    return _modelMatrices;
}

void InstancedModel::draw() const {
    glBindVertexArray(_vao);
    glDrawElementsInstanced(
        GL_TRIANGLES, static_cast<GLsizei>(_indices.size()), GL_UNSIGNED_INT, 0,
        static_cast<GLsizei>(_modelMatrices.size()));
    glBindVertexArray(0);
}

void InstancedModel::draw(int amount) const {
    glBindVertexArray(_vao);
    glDrawElementsInstanced(
        GL_TRIANGLES, static_cast<GLsizei>(_indices.size()), GL_UNSIGNED_INT, 0, amount);
    glBindVertexArray(0);
}

void InstancedModel::drawBoundingBox() const {
    glBindVertexArray(_boxVao);
    glDrawElementsInstanced(
        GL_LINES, 24, GL_UNSIGNED_INT, 0, static_cast<GLsizei>(_modelMatrices.size()));
    glBindVertexArray(0);
}

void InstancedModel::drawBoundingBox(int amount) const {
    glBindVertexArray(_boxVao);
    glDrawElementsInstanced(GL_LINES, 24, GL_UNSIGNED_INT, 0, amount);
    glBindVertexArray(0);
}

GLuint InstancedModel::getInstacenVbo() const {
    return _instanceVbo;
}