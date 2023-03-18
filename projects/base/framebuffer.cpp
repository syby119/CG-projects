#include "framebuffer.h"
#include <stdexcept>

Framebuffer::Framebuffer() {
    glGenFramebuffers(1, &_handle);
}

Framebuffer::Framebuffer(Framebuffer&& rhs) noexcept {
    _handle = rhs._handle;
    rhs._handle = 0;
}

Framebuffer::~Framebuffer() {
    if (_handle != 0) {
        glDeleteFramebuffers(1, &_handle);
        _handle = 0;
    }
}

void Framebuffer::bind() {
    glBindFramebuffer(GL_FRAMEBUFFER, _handle);
}

void Framebuffer::unbind() {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Framebuffer::attachTexture(const Texture& texture, GLenum attachment, int level) {
#ifdef __EMSCRIPTEN__
    std::cerr << "glframebufferTexture2D is not available on WebGL2.0, "
                 "use Framebuffer::attachTexture2D instead"
              << std::endl;
    throw std::logic_error("Not implemented");
#else
    glFramebufferTexture(GL_FRAMEBUFFER, attachment, texture.getHandle(), level);
#endif
}

void Framebuffer::attachTexture2D(
    const Texture& texture, GLenum attachment, GLenum textarget, int level) {
    glFramebufferTexture2D(GL_FRAMEBUFFER, attachment, textarget, texture.getHandle(), level);
}

void Framebuffer::attachTextureLayer(
    const Texture& texture, GLenum attachment, int layer, int level) {
    glFramebufferTextureLayer(GL_FRAMEBUFFER, attachment, texture.getHandle(), level, layer);
}

GLenum Framebuffer::checkStatus(GLenum target) const {
    return glCheckFramebufferStatus(target);
}

std::string Framebuffer::getDiagnostic(GLenum status) const {
    switch (status) {
    case GL_FRAMEBUFFER_COMPLETE: return "framebuffer: complete";
    case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT: return "framebuffer: incomplete attachment";
    case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT: return "framebuffer: missing attachment";
#ifdef USE_GLES
    case GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS: return "framebuffer: incomplete dimensions";
#else
    case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER: return "framebuffer: incomplete draw buffer";
    case GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS: return "framebuffer: incomplete layer targets";
    case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE: return "framebuffer: incomplete multisample";
    case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER: return "framebuffer: incomplete read buffer";
#endif
    case GL_FRAMEBUFFER_UNSUPPORTED: return "framebuffer: unsupported";
    case GL_FRAMEBUFFER_UNDEFINED: return "framebuffer: undefined";
    }

    return "framebuffer: unknown";
}

void Framebuffer::drawBuffer(GLenum buffer) const {
#ifndef USE_GLES
    glDrawBuffer(buffer);
#else
    glDrawBuffers(static_cast<GLsizei>(1), &buffer);
#endif
}

void Framebuffer::drawBuffers(const std::vector<GLenum>& buffers) const {
    glDrawBuffers(static_cast<GLsizei>(buffers.size()), buffers.data());
}

void Framebuffer::readBuffer(GLenum buffer) const {
    glReadBuffer(buffer);
}

GLuint Framebuffer::getHandle() const {
    return _handle;
}