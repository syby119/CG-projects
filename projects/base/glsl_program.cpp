#include <iostream>
#include <fstream>
#include <sstream>
#include <stdexcept>

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
      _fragmentShaders(std::move(rhs._fragmentShaders)) {
    rhs._handle = 0;
    rhs._vertexShaders.clear();
    rhs._fragmentShaders.clear();
}

GLSLProgram::~GLSLProgram() {
    for (const auto vertexShader : _vertexShaders) {
        glDeleteShader(vertexShader);
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

void GLSLProgram::attachFragmentShader(const std::string& code) {
    GLuint fragmentShader = createShader(code, GL_FRAGMENT_SHADER);
    glAttachShader(_handle, fragmentShader);
    _vertexShaders.push_back(fragmentShader);
}

void GLSLProgram::attachVertexShaderFromFile(const std::string& filePath) {
    const std::string& code = readFile(filePath);
    attachVertexShader(code);
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

void GLSLProgram::setBool(const std::string& name, bool value) const {
    GLint location = glGetUniformLocation(_handle, name.c_str());
    if (location == -1) {
        std::cerr << "find uniform " + name + " location failure" << std::endl;
    }

    glUniform1i(location, static_cast<int>(value));
}

void GLSLProgram::setInt(const std::string& name, int value) const {
    GLint location = glGetUniformLocation(_handle, name.c_str());
    if (location == -1) {
        std::cerr << "find uniform " + name + " location failure" << std::endl;
    }

    glUniform1i(location, value);
}

void GLSLProgram::setFloat(const std::string& name, float value) const {
    GLint location = glGetUniformLocation(_handle, name.c_str());
    if (location == -1) {
        std::cerr << "find uniform " + name + " location failure" << std::endl;
    }

    glUniform1f(location, value);
}

void GLSLProgram::setVec2(const std::string& name, const glm::vec2& v2) const {
    GLint location = glGetUniformLocation(_handle, name.c_str());
    if (location == -1) {
        std::cerr << "find uniform " + name + " location failure" << std::endl;
    }

    glUniform2fv(location, 1, &v2[0]);
}

void GLSLProgram::setVec3(const std::string& name, const glm::vec3& v3) const {
    GLint location = glGetUniformLocation(_handle, name.c_str());
    if (location == -1) {
        std::cerr << "find uniform " + name + " location failure" << std::endl;
    }

    glUniform3fv(location, 1, &v3[0]);
}

void GLSLProgram::setVec4(const std::string& name, const glm::vec4& v4) const {
    GLint location = glGetUniformLocation(_handle, name.c_str());
    if (location == -1) {
        std::cerr << "find uniform " + name + " location failure" << std::endl;
    }

    glUniform4fv(location, 1, &v4[0]);
}

void GLSLProgram::setMat3(const std::string& name, const glm::mat3& mat3) const {
    GLint location = glGetUniformLocation(_handle, name.c_str());
    if (location == -1) {
        std::cerr << "find uniform " + name + " location failure" << std::endl;
    }

    glUniform3fv(location, 1, &mat3[0][0]);
}

void GLSLProgram::setMat4(const std::string& name, const glm::mat4& mat4) const {
    GLint location = glGetUniformLocation(_handle, name.c_str());
    if (location == -1) {
        std::cerr << "find uniform " + name + " location failure" << std::endl;
    }

    glUniformMatrix4fv(location, 1, GL_FALSE, &mat4[0][0]);
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
        throw std::runtime_error(std::string("read ") + filePath + "error: " + e.what());
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