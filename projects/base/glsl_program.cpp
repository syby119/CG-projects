#include <iostream>
#include <fstream>
#include <sstream>
#include <stdexcept>

#include <glm/ext.hpp>

#include "glsl_program.h"

GLSLProgram::GLSLProgram() {
    _handle = glCreateProgram();
    if (_handle == 0) {
        throw std::runtime_error("create glsl program failure");
    }
}

GLSLProgram::GLSLProgram(GLSLProgram&& rhs) noexcept
    : _handle(rhs._handle),
      _vertexShaders(std::move(rhs._vertexShaders)),
      _geometryShaders(std::move(rhs._geometryShaders)),
      _fragmentShaders(std::move(rhs._fragmentShaders)) {
    rhs._handle = 0;
    rhs._vertexShaders.clear();
    rhs._geometryShaders.clear();
    rhs._fragmentShaders.clear();
}

GLSLProgram::~GLSLProgram() {
    for (const auto vertexShader : _vertexShaders) {
        glDeleteShader(vertexShader);
    }

    for (const auto geometryShader : _geometryShaders) {
        glDeleteShader(geometryShader);
    }

    for (const auto fragmentShader : _fragmentShaders) {
        glDeleteShader(fragmentShader);
    }

    if (_handle) {
        glDeleteProgram(_handle);
        _handle = 0;
    }
}

void GLSLProgram::attachVertexShader(const std::string& code) {
    GLuint vertexShader = createShader(code, GL_VERTEX_SHADER);
    glAttachShader(_handle, vertexShader);
    _vertexShaders.push_back(vertexShader);
}

void GLSLProgram::attachGeometryShader(const std::string& code) {
    GLuint geometryShader = createShader(code, GL_GEOMETRY_SHADER);
    glAttachShader(_handle, geometryShader);
    _geometryShaders.push_back(geometryShader);
}

void GLSLProgram::attachFragmentShader(const std::string& code) {
    GLuint fragmentShader = createShader(code, GL_FRAGMENT_SHADER);
    glAttachShader(_handle, fragmentShader);
    _fragmentShaders.push_back(fragmentShader);
}

void GLSLProgram::attachVertexShaderFromFile(const std::string& filePath) {
    const std::string& code = readFile(filePath);
    attachVertexShader(code);
}

void GLSLProgram::attachGeometryShaderFromFile(const std::string& filePath) {
    const std::string& code = readFile(filePath);
    attachGeometryShader(code);
}

void GLSLProgram::attachFragmentShaderFromFile(const std::string& filePath) {
    const std::string& code = readFile(filePath);
    attachFragmentShader(code);
}

void GLSLProgram::setTransformFeedbackVaryings(
    const std::vector<const char*>& varyings, GLenum bufferMode) {
    glTransformFeedbackVaryings(_handle, static_cast<GLsizei>(varyings.size()), 
                                varyings.data(), bufferMode);
}

void GLSLProgram::link() {
    glLinkProgram(_handle);

    GLint success;
    glGetProgramiv(_handle, GL_LINK_STATUS, &success);
    if (!success) {
        char buffer[1024];
        glGetProgramInfoLog(_handle, sizeof(buffer), NULL, buffer);
        throw std::runtime_error("link program error: " + std::string(buffer));
    }
}

void GLSLProgram::use() {
    glUseProgram(_handle);
}

int GLSLProgram::getUniformBlockSize(const std::string& name) const {
    GLuint blockIndex = glGetUniformBlockIndex(_handle, name.c_str());
    if (blockIndex == GL_INVALID_INDEX) {
        return -1;
    }

    GLint blockSize;
    glGetActiveUniformBlockiv(_handle, blockIndex, GL_UNIFORM_BLOCK_DATA_SIZE, &blockSize);

    return blockSize;
}

int GLSLProgram::getUniformBlockIndex(const std::string& name) const {
    GLuint index = glGetUniformBlockIndex(_handle, name.c_str());
    if (index == GL_INVALID_INDEX) {
        return -1;
    }

    return index;
}

int GLSLProgram::getUniformBlockVariableOffset(const std::string& name) const {
    GLuint index;
    const char* queryNames[] = { name.c_str()};
    glGetUniformIndices(_handle, 1, queryNames, &index);
    if (index == GL_INVALID_INDEX) {
        return -1;
    }

    GLint offset;
    glGetActiveUniformsiv(_handle, 1, &index, GL_UNIFORM_OFFSET, &offset);

    return offset;
}

void GLSLProgram::setUniformBool(const std::string& name, bool value) const {
    GLint location = glGetUniformLocation(_handle, name.c_str());
    if (location == -1) {
        std::cerr << "find uniform " + name + " location failure" << std::endl;
    }

    glUniform1i(location, static_cast<int>(value));
}

void GLSLProgram::setUniformInt(const std::string& name, int value) const {
    GLint location = glGetUniformLocation(_handle, name.c_str());
    if (location == -1) {
        std::cerr << "find uniform " + name + " location failure" << std::endl;
    }

    glUniform1i(location, value);
}

void GLSLProgram::setUniformUint(const std::string& name, uint32_t value) const {
    GLint location = glGetUniformLocation(_handle, name.c_str());
    if (location == -1) {
        std::cerr << "find uniform " + name + " location failure" << std::endl;
    }

    glUniform1ui(location, value);
}

void GLSLProgram::setUniformFloat(const std::string& name, float value) const {
    GLint location = glGetUniformLocation(_handle, name.c_str());
    if (location == -1) {
        std::cerr << "find uniform " + name + " location failure" << std::endl;
    }

    glUniform1f(location, value);
}

void GLSLProgram::setUniformVec2(const std::string& name, const glm::vec2& v2) const {
    GLint location = glGetUniformLocation(_handle, name.c_str());
    if (location == -1) {
        std::cerr << "find uniform " + name + " location failure" << std::endl;
    }

    glUniform2fv(location, 1, glm::value_ptr(v2));
}

void GLSLProgram::setUniformVec3(const std::string& name, const glm::vec3& v3) const {
    GLint location = glGetUniformLocation(_handle, name.c_str());
    if (location == -1) {
        std::cerr << "find uniform " + name + " location failure" << std::endl;
    }

    glUniform3fv(location, 1, glm::value_ptr(v3));
}

void GLSLProgram::setUniformVec4(const std::string& name, const glm::vec4& v4) const {
    GLint location = glGetUniformLocation(_handle, name.c_str());
    if (location == -1) {
        std::cerr << "find uniform " + name + " location failure" << std::endl;
    }

    glUniform4fv(location, 1, glm::value_ptr(v4));
}

void GLSLProgram::setUniformMat2(const std::string& name, const glm::mat2& mat2) const {
    GLint location = glGetUniformLocation(_handle, name.c_str());
    if (location == -1) {
        std::cerr << "find uniform " + name + " location failure" << std::endl;
    }

    glUniform3fv(location, 1, glm::value_ptr(mat2));
}

void GLSLProgram::setUniformMat3(const std::string& name, const glm::mat3& mat3) const {
    GLint location = glGetUniformLocation(_handle, name.c_str());
    if (location == -1) {
        std::cerr << "find uniform " + name + " location failure" << std::endl;
    }

    glUniform3fv(location, 1, glm::value_ptr(mat3));
}

void GLSLProgram::setUniformMat4(const std::string& name, const glm::mat4& mat4) const {
    GLint location = glGetUniformLocation(_handle, name.c_str());
    if (location == -1) {
        std::cerr << "find uniform " + name + " location failure" << std::endl;
    }

    glUniformMatrix4fv(location, 1, GL_FALSE, glm::value_ptr(mat4));
}

void GLSLProgram::setUniformBlockBinding(const std::string& name, uint32_t binding) const {
    GLuint blockIndex = glGetUniformBlockIndex(_handle, name.c_str());
    if (blockIndex == GL_INVALID_INDEX) {
        std::cerr << "find uniform block " + name + " index failure" << std::endl;
    }

    glUniformBlockBinding(_handle, blockIndex, binding);
}

std::string GLSLProgram::readFile(const std::string& filePath) {
    std::ifstream is;
    is.exceptions(std::ifstream::failbit | std::ifstream::badbit);

    try {
        is.open(filePath);
        std::stringstream ss;
        ss << is.rdbuf();

        return ss.str();
    }
    catch (std::ifstream::failure& e) {
        throw std::runtime_error(std::string("read ") + filePath + " error: " + e.what());
    }
}

GLuint GLSLProgram::createShader(const std::string& code, GLenum shaderType) {
    GLuint shader = glCreateShader(shaderType);
    if (shader == 0) {
        throw std::runtime_error("create shader failure");
    }

    const char* codeBuf = code.c_str();
    glShaderSource(shader, 1, &codeBuf, nullptr);
    glCompileShader(shader);

    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char buffer[1024];
        glGetShaderInfoLog(shader, sizeof(buffer), nullptr, buffer);
        std::cerr << code << std::endl;
        throw std::runtime_error("compile error: \n" + std::string(buffer));
    }

    return shader;
}