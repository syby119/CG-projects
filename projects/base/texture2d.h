#pragma once

#include <string>
#include <glad/glad.h>

#include "texture.h"

class Texture2D : public Texture {
public:
    Texture2D() = default;

	Texture2D(GLenum internalFormat, 
		int width, int height, GLenum format, GLenum dataType, void* data = nullptr);

	Texture2D(Texture2D&& rhs) noexcept;

	~Texture2D() = default;

	void bind(int slot = 0) const override;

	void unbind() const override;

	void generateMipmap() const override;

	void setParamterInt(GLenum name, int value) const override;

	void setParamterFloatVector(GLenum name, const std::vector<float>& values);

private:
    void setDefaultParameters();
};

class ImageTexture2D : public Texture2D {
public:
	ImageTexture2D(const std::string& path);

	ImageTexture2D(
		const void* data,
		int width, 
		int height, 
		int channels,
		GLint internalformat, 
		GLint format, 
		GLenum type,
		const std::string& uri);

	ImageTexture2D(ImageTexture2D&& rhs) noexcept;

	~ImageTexture2D() = default;

	const std::string& getUri() const;

private:
	std::string _uri;

	void setDefaultParameters();

	void upload(
		const void* data,
		int width,
		int height,
		int channels,
		GLint internalformat,
		GLint format,
		GLenum type);
};

class Texture2DArray: public Texture {
public:
    Texture2DArray(
        GLenum internalFormat, int width, int height, int layers, GLenum format, GLenum dataType);

    Texture2DArray(Texture2DArray&& rhs) noexcept;

    Texture2DArray() = default;

    void bind(int slot = 0) const override;

	void unbind() const override;

	void generateMipmap() const override;

	void setParamterInt(GLenum name, int value) const override;

	void setParamterFloatVector(GLenum name, const std::vector<float>& values) const;

private:
    void setDefaultParameters();
};