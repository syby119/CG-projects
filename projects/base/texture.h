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

	virtual void bind(int slot = 0) const = 0;

	virtual void unbind() const = 0;

	virtual void generateMipmap() const = 0;

	virtual void setParamterInt(GLenum name, int value) const = 0;

	GLuint getHandle() const;

protected:
	GLuint _handle = {};

	void check();

	virtual void cleanup();
};

class Texture2D : public Texture {
public:
	Texture2D(const std::string& path);

	Texture2D(
		const void* data,
		int width, 
		int height, 
		int channels,
		GLint internalformat, 
		GLint format, 
		GLenum type,
		const std::string& uri);

	Texture2D(Texture2D&& rhs) noexcept;

	~Texture2D() = default;

	void bind(int slot = 0) const override;

	void unbind() const override;

	void generateMipmap() const override;

	void setParamterInt(GLenum name, int value) const override;

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

class TextureCubemap : public Texture {
public:
	TextureCubemap(const std::vector<std::string>& filenames);

	TextureCubemap(TextureCubemap&& rhs) noexcept;

	~TextureCubemap() = default;

	void bind(int slot = 0) const override;

	void unbind() const override;

	void generateMipmap() const override;

	void setParamterInt(GLenum name, int value) const override;

	const std::vector<std::string>& getUris() const;

private:
	std::vector<std::string> _uris;
};

class DataTexture2D : public Texture {
public:
	DataTexture2D(GLenum internalFormat, int width, int height, GLenum format, GLenum dataType);

	DataTexture2D(DataTexture2D&& rhs) noexcept;

	~DataTexture2D() = default;

	void bind(int slot = 0) const override;

	void unbind() const override;

	void generateMipmap() const override;

	void setParamterInt(GLenum name, int value) const override;
};

class DataTextureCubemap : public Texture {
public:
	DataTextureCubemap(GLenum internalFormat, int width, int height, GLenum format, GLenum dataType);

	DataTextureCubemap(DataTextureCubemap&& rhs) noexcept;

	~DataTextureCubemap() = default;

	void bind(int slot = 0) const override;

	void unbind() const override;

	void generateMipmap() const override;

	void setParamterInt(GLenum name, int value) const override;
};