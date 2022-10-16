#include <cassert>

#include "texture.h"

Texture::Texture() {
	// create texture object
	glGenTextures(1, &_handle);
}

Texture::Texture(Texture&& rhs) noexcept: _handle(rhs._handle) {
	rhs._handle = 0;
}

Texture::~Texture() {
	// destroy texture object
	if (_handle != 0) {
		glDeleteTextures(1, &_handle);
		_handle = 0;
	}
}

GLuint Texture::getHandle() const {
	return _handle;
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
	if (_handle != 0) {
		glDeleteTextures(1, &_handle);
		_handle = 0;
	}
}