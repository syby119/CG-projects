#include <cassert>
#include <sstream>
#include <stb_image.h>

#include "texture2d.h"

Texture2D::Texture2D(
    GLint internalFormat, int width, int height, GLenum format, GLenum dataType, void* data) {
    glBindTexture(GL_TEXTURE_2D, _handle);
    setDefaultParameters();
    glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, format, dataType, data);
    glBindTexture(GL_TEXTURE_2D, 0);
}

Texture2D::Texture2D(Texture2D&& rhs) noexcept : Texture(std::move(rhs)) {}

void Texture2D::bind(int slot) const {
    glActiveTexture(GL_TEXTURE0 + slot);
    glBindTexture(GL_TEXTURE_2D, _handle);
}

void Texture2D::unbind() const {
    glBindTexture(GL_TEXTURE_2D, 0);
}

void Texture2D::generateMipmap() const {
    glGenerateMipmap(GL_TEXTURE_2D);
}

void Texture2D::setParamterInt(GLenum name, int value) const {
    glTexParameteri(GL_TEXTURE_2D, name, value);
}

void Texture2D::setParamterFloatVector(GLenum name, const std::vector<float>& values) {
    glTexParameterfv(GL_TEXTURE_2D, name, values.data());
}

void Texture2D::setDefaultParameters() {
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
}

ImageTexture2D::ImageTexture2D(const std::string& path) : _uri(path) {
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
    case 1: format = GL_RED; break;
    case 3: format = GL_RGB; break;
    case 4: format = GL_RGBA; break;
    default:
        cleanup();
        stbi_image_free(data);
        throw std::runtime_error("unsupported format");
    }
    GLint internalFormat = static_cast<GLint>(format);

    glBindTexture(GL_TEXTURE_2D, _handle);

    // set texture parameters
    setDefaultParameters();

    // transfer the image data to GPU
    upload(data, width, height, channels, internalFormat, format, GL_UNSIGNED_BYTE);

    glBindTexture(GL_TEXTURE_2D, 0);

    // free data
    stbi_image_free(data);

    // check error
    check();
}

ImageTexture2D::ImageTexture2D(
    const void* data, int width, int height, int channels, GLint internalformat, GLenum format,
    GLenum type, const std::string& uri)
    : _uri(uri) {
    glBindTexture(GL_TEXTURE_2D, _handle);

    // set texture parameters
    setDefaultParameters();

    // transfer the image data to GPU
    upload(data, width, height, channels, internalformat, format, type);

    glBindTexture(GL_TEXTURE_2D, 0);

    // check error
    check();
}

ImageTexture2D::ImageTexture2D(ImageTexture2D&& rhs) noexcept
    : Texture2D(std::move(rhs)), _uri(std::move(rhs._uri)) {
    rhs._uri = "";
}

const std::string& ImageTexture2D::getUri() const {
    return _uri;
}

void ImageTexture2D::setDefaultParameters() {
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
}

void ImageTexture2D::upload(
    const void* data, int width, int height, int channels, GLint internalformat, GLenum format,
    GLenum type) {
    // 1. set alignment for data transfer
    GLint alignment = 1;
    size_t pitch = width * channels * sizeof(unsigned char);
    if (pitch % 8 == 0)
        alignment = 8;
    else if (pitch % 4 == 0)
        alignment = 4;
    else if (pitch % 2 == 0)
        alignment = 2;
    else
        alignment = 1;

    glPixelStorei(GL_UNPACK_ALIGNMENT, alignment);

    // 2. transfer data
    glTexImage2D(GL_TEXTURE_2D, 0, internalformat, width, height, 0, format, type, data);

    // 3. restore alignment
    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
}

Texture2DArray::Texture2DArray(
    GLint internalFormat, int width, int height, int layers, GLenum format, GLenum dataType) {
    glBindTexture(GL_TEXTURE_2D_ARRAY, _handle);
    glTexImage3D(
        GL_TEXTURE_2D_ARRAY, 0, internalFormat, width, height, layers, 0, format, dataType,
        nullptr);
    setDefaultParameters();
    glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
}

Texture2DArray::Texture2DArray(Texture2DArray&& rhs) noexcept : Texture(std::move(rhs)) {}

void Texture2DArray::bind(int slot) const {
    glActiveTexture(GL_TEXTURE0 + slot);
    glBindTexture(GL_TEXTURE_2D_ARRAY, _handle);
}

void Texture2DArray::unbind() const {
    glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
}

void Texture2DArray::generateMipmap() const {
    glGenerateMipmap(GL_TEXTURE_2D_ARRAY);
}

void Texture2DArray::setParamterInt(GLenum name, int value) const {
    glTexParameteri(GL_TEXTURE_2D_ARRAY, name, value);
}

void Texture2DArray::setParamterFloatVector(GLenum name, const std::vector<float>& values) const {
    glTexParameterfv(GL_TEXTURE_2D_ARRAY, name, values.data());
}

void Texture2DArray::setDefaultParameters() {
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
}