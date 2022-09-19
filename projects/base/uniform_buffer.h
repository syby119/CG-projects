#pragma once

#include <map>
#include <string>

#include <glad/glad.h>

class UniformBuffer {
public:
	UniformBuffer(size_t bufferSize, GLenum usage) {
		glGenBuffers(1, &_handle);
		glBindBuffer(GL_UNIFORM_BUFFER, _handle);
		glBufferData(GL_UNIFORM_BUFFER, bufferSize, nullptr, usage);
		glBindBuffer(GL_UNIFORM_BUFFER, 0);
	}

	UniformBuffer(UniformBuffer&& rhs) noexcept
		: _handle(rhs._handle), _offsetMap(std::move(rhs._offsetMap)) {
		rhs._handle = 0;
	}

	~UniformBuffer() {
		if (_handle != 0) {
			glDeleteBuffers(1, &_handle);
			_handle = 0;
		}
	}

	void setBindingPoint(uint32_t index) const {
		glBindBufferBase(GL_UNIFORM_BUFFER, index, _handle);
	}

	void setOffset(const std::string& name, size_t offset) {
		_offsetMap[name] = offset;
	}

	template <typename T>
	void update(const std::string& name, const T& value) const {
		const auto iter = _offsetMap.find(name);
		if (iter == _offsetMap.end()) {
			std::cerr << "cannot find " + name + " in the ubo" << std::endl;
			return;
		}

		glBindBuffer(GL_UNIFORM_BUFFER, _handle);
		glBufferSubData(GL_UNIFORM_BUFFER, iter->second, sizeof(T), &value);
		glBindBuffer(GL_UNIFORM_BUFFER, 0);
	}

private:
	GLuint _handle {};
	std::map<std::string, size_t> _offsetMap;
};

template <>
inline void UniformBuffer::update<bool>(const std::string& name, const bool& value) const {
	const auto iter = _offsetMap.find(name);
	if (iter == _offsetMap.end()) {
		std::cerr << "cannot find " + name + " in the ubo" << std::endl;
		return;
	}

	int intVal = static_cast<int>(value);
	glBindBuffer(GL_UNIFORM_BUFFER, _handle);
	glBufferSubData(GL_UNIFORM_BUFFER, iter->second, sizeof(int), &intVal);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);
}