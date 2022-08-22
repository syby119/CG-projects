#pragma once

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

	void attach(const DataTexture& texture, GLenum attachment, int level = 0) {
		bind();
		glFramebufferTexture2D(GL_FRAMEBUFFER,
			attachment, GL_TEXTURE_2D, texture.getHandle(), level);
		unbind();
	}

	bool isComplete() const {
		return glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE;
	}
private:
	GLuint _handle;
};