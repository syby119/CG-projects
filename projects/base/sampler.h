#pragma once

#include <glad/glad.h>

class Sampler {
public:
    Sampler() {
        glGenSamplers(1, &_handle);
    }
    
    Sampler(Sampler&& rhs) noexcept {
        rhs._handle = _handle;
        _handle = 0;
    }

    ~Sampler() {
        if (_handle != 0) {
            glDeleteSamplers(1, &_handle);
        }
    }

    void setInt(GLenum pname, int param) {
        glSamplerParameteri(_handle, pname, param);
    }

    void setFloat(GLenum pname, float param) {
        glSamplerParameterf(_handle, pname, param);
    }

    void setIntVec(GLenum pname, int* param) {
        glSamplerParameteriv(_handle, pname, param);
    }

    void setFloatVec(GLenum pname, float* param) {
        glSamplerParameterfv(_handle, pname, param);
    }

    void bind(GLuint texUnit) const {
        glBindSampler(texUnit, _handle);
    }

    void unbind(GLuint texUnit) const {
        glBindSampler(texUnit, 0);
    }

private:
    GLuint _handle = 0;
};