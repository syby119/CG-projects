#include <cassert>

#include "texture.h"

Texture::Texture() {
    // create texture object
    glGenTextures(1, &m_handle);
}

Texture::Texture(Texture&& rhs) noexcept : m_handle(rhs.m_handle) {
    rhs.m_handle = 0;
}

Texture::~Texture() {
    // destroy texture object
    if (m_handle != 0) {
        glDeleteTextures(1, &m_handle);
        m_handle = 0;
    }
}

GLuint Texture::getHandle() const {
    return m_handle;
}

void Texture::check() {
    GLenum error = glGetError();
    if (error != GL_NO_ERROR) {
        std::stringstream ss;
        ss << "texture object operation failure, (code " << error << ")";
        cleanup();
        throw std::runtime_error(ss.str());
    }
}

void Texture::cleanup() {
    if (m_handle != 0) {
        glDeleteTextures(1, &m_handle);
        m_handle = 0;
    }
}