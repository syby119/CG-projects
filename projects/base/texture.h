#pragma once

#include <string>
#include <sstream>
#include <vector>

#include <glad/glad.h>
#include <stb_image.h>

class Texture {
public:
	Texture();

	Texture(Texture&& rhs) noexcept;

	virtual ~Texture();

	virtual void bind() const = 0;

	virtual void unbind() const = 0;

	GLuint getHandle() const;

protected:
	GLuint _handle = {};

	virtual void cleanup();
};

class Texture2D : public Texture {
public:
	Texture2D(const std::string path);

	Texture2D(Texture2D&& rhs) noexcept;

	~Texture2D() = default;

	void bind() const override;

	virtual void unbind() const;

private:
	std::string _path;
};

class TextureCubemap : public Texture {
public:
	TextureCubemap(const std::vector<std::string>& filenames);

	TextureCubemap(TextureCubemap&& rhs) noexcept;

	~TextureCubemap() = default;

	void bind() const override;

	void unbind() const override;

private:
	std::vector<std::string> _paths;
};

class DataTexture : public Texture {
public:
	DataTexture(GLenum internalFormat, int width, int height, GLenum format, GLenum dataType);

	DataTexture(DataTexture&& rhs) noexcept;

	~DataTexture() = default;

	void bind() const override;

	virtual void unbind() const;
};