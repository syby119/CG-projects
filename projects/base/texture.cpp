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

void Texture::cleanup() {
	if (_handle != 0) {
		glDeleteTextures(1, &_handle);
		_handle = 0;
	}
}

Texture2D::Texture2D(const std::string path): _path(path) {
	// load image to the memory
	stbi_set_flip_vertically_on_load(true);
	int width = 0, height = 0, channels = 0;
	unsigned char* data = stbi_load(_path.c_str(), &width, &height, &channels, 0);
	if (data == nullptr) {
		cleanup();
		throw std::runtime_error("load " + path + " failure");
	}

	// choose image format
	GLenum format = GL_RGB;
	switch (channels) {
	case 1: format = GL_RED;  break;
	case 3: format = GL_RGB;  break;
	case 4: format = GL_RGBA; break;
	default:
		cleanup();
		stbi_image_free(data);
		throw std::runtime_error("unsupported format");
	}

	// set texture parameters
	glBindTexture(GL_TEXTURE_2D, _handle);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

	// transfer data to gpu
	// 1. set alignment for data transfer
	GLint alignment = 1;
	size_t pitch = width * channels * sizeof(unsigned char);
	if (pitch % 8 == 0)      alignment = 8;
	else if (pitch % 4 == 0) alignment = 4;
	else if (pitch % 2 == 0) alignment = 2;
	else                     alignment = 1;

	glPixelStorei(GL_UNPACK_ALIGNMENT, alignment);

	// 2. transfer data
	glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);

	// 3. restore alignment
	glPixelStorei(GL_UNPACK_ALIGNMENT, 4);

	// unbind texture
	glBindTexture(GL_TEXTURE_2D, 0);

	// free data
	stbi_image_free(data);

	GLenum error = glGetError();
	if (error != GL_NO_ERROR) {
		std::stringstream ss;
		ss << "texture object operation failure, (code " << error << ")";
		cleanup();
		throw std::runtime_error(ss.str());
	}
}

Texture2D::Texture2D(Texture2D&& rhs) noexcept
	: Texture(std::move(rhs)), 
	  _path(std::move(rhs._path)) {
	rhs._path = "";
}

void Texture2D::bind() const {
	glBindTexture(GL_TEXTURE_2D, _handle);
}

void Texture2D::unbind() const {
	glBindTexture(GL_TEXTURE_2D, 0);
}

TextureCubemap::TextureCubemap(const std::vector<std::string>& filenames)
	: _paths(filenames) {
	assert(filenames.size() == 6);
	// TODO: load 6 images and generate texture cubemap
	// hint: you can refer to Texture2D(const std::string&) for image loading
	// write your code here
	// -----------------------------------------------
	// ...
	// -----------------------------------------------
}

TextureCubemap::TextureCubemap(TextureCubemap&& rhs) noexcept
	: Texture(std::move(rhs)), 
	  _paths(std::move(rhs._paths)){
	rhs._paths.clear();
}


void TextureCubemap::bind() const {
	glBindTexture(GL_TEXTURE_CUBE_MAP, _handle);
}

void TextureCubemap::unbind() const {
	glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
}

DataTexture::DataTexture(GLenum internalFormat, int width, int height, GLenum format, GLenum dataType) {
	glBindTexture(GL_TEXTURE_2D, _handle);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, format, dataType, 0);
}

DataTexture::DataTexture(DataTexture&& rhs) noexcept
	: Texture(std::move(rhs)) { }

void DataTexture::bind() const {
	glBindTexture(GL_TEXTURE_2D, _handle);
}

void DataTexture::unbind() const {
	glBindTexture(GL_TEXTURE_2D, 0);
}