#pragma once

#include <string>
#include <sstream>
#include <vector>

#include <glad/glad.h>
#include <stb_image.h>
#include <stb_image_write.h>

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