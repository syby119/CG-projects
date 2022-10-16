#pragma once

#include <string>
#include <vector>
#include <glad/glad.h>

#include "texture.h"

class TextureCubemap : public Texture {
public:
    TextureCubemap() = default;

	TextureCubemap(GLenum internalFormat, int width, int height, GLenum format, GLenum dataType);

	TextureCubemap(TextureCubemap&& rhs) noexcept;

	~TextureCubemap() = default;

	void bind(int slot = 0) const override;

	void unbind() const override;

	void generateMipmap() const override;

	void setParamterInt(GLenum name, int value) const override;
};

class ImageTextureCubemap : public TextureCubemap {
public:
	ImageTextureCubemap(const std::vector<std::string>& filepaths);

	ImageTextureCubemap(ImageTextureCubemap&& rhs) noexcept;

	~ImageTextureCubemap() = default;

	const std::vector<std::string>& getUris() const;

private:
	std::vector<std::string> _uris;
};