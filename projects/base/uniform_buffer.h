#pragma once

#include <map>
#include <string>

#include "gl_utility.h"

class UniformBuffer {
public:
    UniformBuffer(size_t bufferSize, GLenum usage) {
        glGenBuffers(1, &m_handle);
        glBindBuffer(GL_UNIFORM_BUFFER, m_handle);
        glBufferData(GL_UNIFORM_BUFFER, bufferSize, nullptr, usage);
        glBindBuffer(GL_UNIFORM_BUFFER, 0);
    }

    UniformBuffer(UniformBuffer&& rhs) noexcept
        : m_handle(rhs.m_handle), m_offsetMap(std::move(rhs.m_offsetMap)) {
        rhs.m_handle = 0;
    }

    ~UniformBuffer() {
        if (m_handle != 0) {
            glDeleteBuffers(1, &m_handle);
            m_handle = 0;
        }
    }

    void setBindingPoint(uint32_t index) const {
        glBindBufferBase(GL_UNIFORM_BUFFER, index, m_handle);
    }

    void setOffset(const std::string& name, size_t offset) {
        m_offsetMap[name] = offset;
    }

    template <typename T>
    void update(const std::string& name, const T& value) const {
        const auto iter = m_offsetMap.find(name);
        if (iter == m_offsetMap.end()) {
            std::cerr << "cannot find " + name + " in the ubo" << std::endl;
            return;
        }

        glBindBuffer(GL_UNIFORM_BUFFER, m_handle);
        glBufferSubData(GL_UNIFORM_BUFFER, iter->second, sizeof(T), &value);
        glBindBuffer(GL_UNIFORM_BUFFER, 0);
    }

private:
    GLuint m_handle{};
    std::map<std::string, size_t> m_offsetMap;
};

template <>
inline void UniformBuffer::update<bool>(const std::string& name, const bool& value) const {
    const auto iter = m_offsetMap.find(name);
    if (iter == m_offsetMap.end()) {
        std::cerr << "cannot find " + name + " in the ubo" << std::endl;
        return;
    }

    int intVal = static_cast<int>(value);
    glBindBuffer(GL_UNIFORM_BUFFER, m_handle);
    glBufferSubData(GL_UNIFORM_BUFFER, iter->second, sizeof(int), &intVal);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
}