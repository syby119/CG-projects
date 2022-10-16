#pragma once

#include <vector>
#include <glad/glad.h>

#include "texture.h"

class Framebuffer {
public:
	Framebuffer() {
		glGenFramebuffers(1, &_handle);
	}

	Framebuffer(const Framebuffer&) = delete;

	Framebuffer(Framebuffer&& rhs) noexcept {
		_handle = rhs._handle;
		rhs._handle = 0;
	}

	~Framebuffer() {
		if (_handle != 0) {
			glDeleteFramebuffers(1, &_handle);
			_handle = 0;
		}
	}

	void bind() {
		glBindFramebuffer(GL_FRAMEBUFFER, _handle);
	}

	void unbind() {
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}

	void attachTexture(const Texture& texture, GLenum attachment, int level = 0) {
		glFramebufferTexture(GL_FRAMEBUFFER, attachment, texture.getHandle(), level);
	}

	void attachTexture2D(const Texture& texture, GLenum attachment, GLenum textarget, int level = 0) {
		glFramebufferTexture2D(GL_FRAMEBUFFER,
			attachment, textarget, texture.getHandle(), level);
	}

	GLenum checkStatus(GLenum target) const {
		return glCheckFramebufferStatus(target);
	}

	void drawBuffer(GLenum buffer) const {
		glDrawBuffer(buffer);
	}

	void drawBuffers(const std::vector<GLenum>& buffers) const {
		glDrawBuffers(static_cast<GLsizei>(buffers.size()), buffers.data());
	}

	void readBuffer(GLenum buffer) const {
		glReadBuffer(buffer);
	}

private:
	GLuint _handle;
};