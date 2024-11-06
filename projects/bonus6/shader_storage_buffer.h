#pragma once

#include "../base/glsl_program.h"

class ShaderStorageBuffer {
public:
    ShaderStorageBuffer() {
        glGenBuffers(1, &m_handle);
    }

    ShaderStorageBuffer(ShaderStorageBuffer&& rhs) noexcept
        : m_handle{ std::move(rhs.m_handle) } {
        rhs.m_handle = 0;
    }

    ~ShaderStorageBuffer() {
        if (m_handle) {
            glDeleteBuffers(1, &m_handle);
        }
    }

    ShaderStorageBuffer& operator=(ShaderStorageBuffer&& rhs) noexcept {
        if (&rhs != this) {
            if (m_handle) {
                glDeleteBuffers(1, &m_handle);
            }

            m_handle = rhs.m_handle;
            rhs.m_handle = 0;
        }

        return *this;
    };

    void bind() const {
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_handle);
    }

    static void unbind() {
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    }

    void setBindingPoint(uint32_t index) const {
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, index, m_handle);
    }

    void upload(GLenum usage, size_t size, const void* data = nullptr) {
        glBufferData(GL_SHADER_STORAGE_BUFFER, static_cast<GLsizei>(size), data, usage);
    }

    void* map(GLenum access) {
        return glMapBuffer(GL_SHADER_STORAGE_BUFFER, access);
    }

    void unmap() {
        glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
    }

    GLuint getNativeHandle() const noexcept {
        return m_handle;
    }

private:
    GLuint m_handle{ 0 };
};