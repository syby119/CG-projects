#pragma once

#include <string>
#include <vector>

#include "gl_utility.h"
#include "texture.h"

class Framebuffer {
public:
    Framebuffer();

    Framebuffer(const Framebuffer&) = delete;

    Framebuffer(Framebuffer&& rhs) noexcept;

    ~Framebuffer();

    void bind();

    void unbind();

    void attachTexture(const Texture& texture, GLenum attachment, int level = 0);

    void attachTexture2D(
        const Texture& texture, GLenum attachment, GLenum textarget, int level = 0);

    void attachTextureLayer(const Texture& texture, GLenum attachment, int layer, int level = 0);

    GLenum checkStatus(GLenum target = GL_FRAMEBUFFER) const;

    std::string getDiagnostic(GLenum status) const;

    void drawBuffer(GLenum buffer) const;

    void drawBuffers(const std::vector<GLenum>& buffers) const;

    void readBuffer(GLenum buffer) const;

    GLuint getHandle() const;

private:
    GLuint _handle;
};