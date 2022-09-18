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




Texture2D::Texture2D(const std::string& path): _uri(path) {
	// load image to the memory
	stbi_set_flip_vertically_on_load(true);
	int width = 0, height = 0, channels = 0;
	unsigned char* data = stbi_load(path.c_str(), &width, &height, &channels, 0);
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
	setDefaultParameters();

	// transfer the image data to GPU
	upload(data, width, height, channels, format, format, GL_UNSIGNED_BYTE);

	// free data
	stbi_image_free(data);

	// check error
	check();
}

Texture2D::Texture2D(
	const void* data, 
	int width, 
	int height, 
	int channels,
	GLint internalformat, 
	GLint format, 
	GLenum type,
	const std::string& uri): _uri(uri) {
	// set texture parameters
	setDefaultParameters();

	// transfer the image data to GPU
	upload(data, width, height, channels, internalformat, format, type);

	// check error
	check();
}

Texture2D::Texture2D(Texture2D&& rhs) noexcept
	: Texture(std::move(rhs)), 
	  _uri(std::move(rhs._uri)) {
	rhs._uri = "";
}

void Texture2D::bind(int slot) const {
	glActiveTexture(GL_TEXTURE0 + slot);
	glBindTexture(GL_TEXTURE_2D, _handle);
}

void Texture2D::unbind() const {
	glBindTexture(GL_TEXTURE_2D, 0);
}

void Texture2D::generateMipmap() const {
	glBindTexture(GL_TEXTURE_2D, _handle);
	glGenerateMipmap(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, 0);
}

void Texture2D::setParamterInt(GLenum name, int value) const {
	glBindTexture(GL_TEXTURE_2D, _handle);
	glTexParameteri(GL_TEXTURE_2D, name, value);
	glBindTexture(GL_TEXTURE_2D, 0);
}

const std::string& Texture2D::getUri() const {
	return _uri;
}

void Texture2D::setDefaultParameters() {
	glBindTexture(GL_TEXTURE_2D, _handle);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glBindTexture(GL_TEXTURE_2D, 0);
}

void Texture2D::upload(
	const void* data,
	int width,
	int height,
	int channels,
	GLint internalformat,
	GLint format,
	GLenum type) {
	// transfer data to gpu
	glBindTexture(GL_TEXTURE_2D, _handle);
	// 1. set alignment for data transfer
	GLint alignment = 1;
	size_t pitch = width * channels * sizeof(unsigned char);
	if (pitch % 8 == 0)      alignment = 8;
	else if (pitch % 4 == 0) alignment = 4;
	else if (pitch % 2 == 0) alignment = 2;
	else                     alignment = 1;

	glPixelStorei(GL_UNPACK_ALIGNMENT, alignment);

	// 2. transfer data
	glTexImage2D(GL_TEXTURE_2D, 0, internalformat, width, height, 0, format, type, data);

	// 3. restore alignment
	glPixelStorei(GL_UNPACK_ALIGNMENT, 4);

	// unbind texture
	glBindTexture(GL_TEXTURE_2D, 0);
}




TextureCubemap::TextureCubemap(const std::vector<std::string>& filepaths)
	: _uris(filepaths) {
	assert(filepaths.size() == 6);
	// TODO: load six images and generate the texture cubemap
	// hint: you can refer to Texture2D(const std::string&) for image loading
	// write your code here
	// -----------------------------------------------
	// ...
	// -----------------------------------------------
}

TextureCubemap::TextureCubemap(TextureCubemap&& rhs) noexcept
	: Texture(std::move(rhs)), 
	  _uris(std::move(rhs._uris)){
	rhs._uris.clear();
}

void TextureCubemap::bind(int slot) const {
	glActiveTexture(GL_TEXTURE0 + slot);
	glBindTexture(GL_TEXTURE_CUBE_MAP, _handle);
}

void TextureCubemap::unbind() const {
	glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
}

void TextureCubemap::generateMipmap() const {
	glBindTexture(GL_TEXTURE_CUBE_MAP, _handle);
	glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
	glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
}

void TextureCubemap::setParamterInt(GLenum name, int value) const {
	glBindTexture(GL_TEXTURE_CUBE_MAP, _handle);
	glTextureParameteri(GL_TEXTURE_CUBE_MAP, name, value);
	glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
}

const std::vector<std::string>& TextureCubemap::getUris() const {
	return _uris;
}



DataTexture2D::DataTexture2D(
	GLenum internalFormat, int width, int height, GLenum format, GLenum dataType
) {
	glBindTexture(GL_TEXTURE_2D, _handle);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, format, dataType, 0);
}

DataTexture2D::DataTexture2D(DataTexture2D&& rhs) noexcept
	: Texture(std::move(rhs)) { }

void DataTexture2D::bind(int slot) const {
	glActiveTexture(GL_TEXTURE0 + slot);
	glBindTexture(GL_TEXTURE_2D, _handle);
}

void DataTexture2D::unbind() const {
	glBindTexture(GL_TEXTURE_2D, 0);
}

void DataTexture2D::generateMipmap() const {
	glBindTexture(GL_TEXTURE_2D, _handle);
	glGenerateMipmap(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, 0);
}

void DataTexture2D::setParamterInt(GLenum name, int value) const {
	glBindTexture(GL_TEXTURE_2D, _handle);
	glTexParameteri(GL_TEXTURE_2D, name, value);
	glBindTexture(GL_TEXTURE_2D, 0);
}




DataTextureCubemap::DataTextureCubemap(
	GLenum internalFormat, int width, int height, GLenum format, GLenum dataType
) {
	glBindTexture(GL_TEXTURE_CUBE_MAP, _handle);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	for (uint32_t i = 0; i < 6; ++i) {
		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
			0, internalFormat, width, height, 0, format, dataType, nullptr);
	}

	glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
}

DataTextureCubemap::DataTextureCubemap(DataTextureCubemap&& rhs) noexcept
	: Texture(std::move(rhs)) { }

void DataTextureCubemap::bind(int slot) const {
	glActiveTexture(GL_TEXTURE0 + slot);
	glBindTexture(GL_TEXTURE_CUBE_MAP, _handle);
}

void DataTextureCubemap::unbind() const {
	glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
}

void DataTextureCubemap::generateMipmap() const { 
	glBindTexture(GL_TEXTURE_CUBE_MAP, _handle);
	glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
	glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
}

void DataTextureCubemap::setParamterInt(GLenum name, int value) const { 
	glBindTexture(GL_TEXTURE_CUBE_MAP, _handle);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, name, value);
	glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
}