#pragma once

#include "gl_utility.h"

class Sampler {
public:
    Sampler() {
        glGenSamplers(1, &m_handle);
    }

    Sampler(Sampler&& rhs) noexcept {
        rhs.m_handle = m_handle;
        m_handle = 0;
    }

    ~Sampler() {
        if (m_handle != 0) {
            glDeleteSamplers(1, &m_handle);
        }
    }

    void setInt(GLenum pname, int param) {
        glSamplerParameteri(m_handle, pname, param);
    }

    void setFloat(GLenum pname, float param) {
        glSamplerParameterf(m_handle, pname, param);
    }

    void setIntVec(GLenum pname, int* param) {
        glSamplerParameteriv(m_handle, pname, param);
    }

    void setFloatVec(GLenum pname, float* param) {
        glSamplerParameterfv(m_handle, pname, param);
    }

    void bind(GLuint texUnit) const {
        glBindSampler(texUnit, m_handle);
    }

    void unbind(GLuint texUnit) const {
        glBindSampler(texUnit, 0);
    }

private:
    GLuint m_handle = 0;
};