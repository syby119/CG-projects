#pragma once

#include "gl_program.h"
#include <glm/ext.hpp>

GLProgram::GLProgram() : m_handle{ glCreateProgram() } {
    if (!m_handle) {
        std::runtime_error("Create ...");
    }
}

GLProgram::GLProgram(GLProgram&& rhs) noexcept : m_handle{ rhs.m_handle } {
    rhs.m_handle = 0;
}

GLProgram::~GLProgram() {
    if (m_handle) {
        glDeleteProgram(m_handle);
    }
}

GLProgram& GLProgram::operator=(GLProgram&& rhs) noexcept {
    if (this != &rhs) {
        if (m_handle) {
            glDeleteProgram(m_handle);
        }

        m_handle = rhs.m_handle;
        rhs.m_handle = 0;
    }

    return *this;
}

void GLProgram::attach(ShaderModule const& shaderModule) {
    glAttachShader(m_handle, shaderModule.getHandle());
}

void GLProgram::detach(ShaderModule const& shaderModule) {
    glDetachShader(m_handle, shaderModule.getHandle());
}

void GLProgram::link() {
    glLinkProgram(m_handle);

    GLint result = GL_FALSE;
    glGetProgramiv(m_handle, GL_LINK_STATUS, &result);
    if (result == GL_FALSE) {
        GLint infoLogLength{ 0 };
        glGetProgramiv(m_handle, GL_INFO_LOG_LENGTH, &infoLogLength);

        std::vector<char> buffer(infoLogLength);
        glGetProgramInfoLog(m_handle, infoLogLength, NULL, &buffer[0]);
        throw std::runtime_error(std::string(buffer.data()));
    }
}

void GLProgram::use() {
    glUseProgram(m_handle);
}

void GLProgram::unuse() {
    glUseProgram(0);
}

void GLProgram::setUniform(int location, bool value) const {
    glUniform1i(location, static_cast<int>(value));
}

void GLProgram::setUniform(int location, int value) const {
    glUniform1i(location, value);
}

void GLProgram::setUniform(int location, uint32_t value) const {
    glUniform1ui(location, value);
}

void GLProgram::setUniform(int location, float value) const {
    glUniform1f(location, value);
}

void GLProgram::setUniform(int location, const glm::vec2& v2) const {
    glUniform2fv(location, 1, glm::value_ptr(v2));
}

void GLProgram::setUniform(int location, const glm::vec3& v3) const {
    glUniform3fv(location, 1, glm::value_ptr(v3));
}

void GLProgram::setUniform(int location, const glm::vec4& v4) const {
    glUniform4fv(location, 1, glm::value_ptr(v4));
}

void GLProgram::setUniform(int location, const glm::mat2& mat2) const {
    glUniformMatrix2fv(location, 1, GL_FALSE, glm::value_ptr(mat2));
}

void GLProgram::setUniform(int location, const glm::mat3& mat3) const {
    glUniformMatrix3fv(location, 1, GL_FALSE, glm::value_ptr(mat3));
}

void GLProgram::setUniform(int location, const glm::mat4& mat4) const {
    glUniformMatrix4fv(location, 1, GL_FALSE, glm::value_ptr(mat4));
}