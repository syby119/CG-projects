#include "gl_program.h"

#include <magic_enum/magic_enum.hpp>
#include <glm/ext.hpp>

GLProgram::GLProgram() : m_handle{ glCreateProgram() } {
    if (!m_handle) {
        throw std::runtime_error("Create ...");
    }
}

GLProgram::GLProgram(GLProgram&& rhs) noexcept : m_handle{ rhs.m_handle } {
    rhs.m_handle = 0;
}

GLProgram::~GLProgram() {
    if (m_handle) {
        glDeleteProgram(m_handle);
    }
}

GLProgram& GLProgram::operator=(GLProgram&& rhs) noexcept {
    if (this != &rhs) {
        if (m_handle) {
            glDeleteProgram(m_handle);
        }

        m_handle = rhs.m_handle;
        rhs.m_handle = 0;
    }

    return *this;
}

void GLProgram::attach(ShaderModule const& shaderModule) {
    glAttachShader(m_handle, shaderModule.getHandle());
}

void GLProgram::detach(ShaderModule const& shaderModule) {
    glDetachShader(m_handle, shaderModule.getHandle());
}

void GLProgram::link() {
    glLinkProgram(m_handle);

    GLint result = GL_FALSE;
    glGetProgramiv(m_handle, GL_LINK_STATUS, &result);
    if (result == GL_FALSE) {
        GLint infoLogLength{ 0 };
        glGetProgramiv(m_handle, GL_INFO_LOG_LENGTH, &infoLogLength);

        std::vector<char> buffer(infoLogLength);
        glGetProgramInfoLog(m_handle, infoLogLength, NULL, &buffer[0]);
        throw std::runtime_error(std::string(buffer.data()));
    }
}

void GLProgram::use() {
    glUseProgram(m_handle);
}

void GLProgram::unuse() {
    glUseProgram(0);
}

int GLProgram::getUniformVarLocation(std::string_view name) const {
    for (auto const& [uName, info] : m_uniformVarInfos) {
        if (uName == name) {
            return info.location;
        }
    }

    return -1;
}

int GLProgram::getTextureBinding(std::string_view name) const {
    for (auto const& [tName, info] : m_textureInfos) {
        if (tName == name) {
            return info.binding;
        }
    }

    return 0;
}

void GLProgram::setUniform(int location, bool value) const {
    glUniform1i(location, static_cast<int>(value));
}

void GLProgram::setUniform(int location, int value) const {
    glUniform1i(location, value);
}

void GLProgram::setUniform(int location, uint32_t value) const {
    glUniform1ui(location, value);
}

void GLProgram::setUniform(int location, float value) const {
    glUniform1f(location, value);
}

void GLProgram::setUniform(int location, const glm::vec2& v2) const {
    glUniform2fv(location, 1, glm::value_ptr(v2));
}

void GLProgram::setUniform(int location, const glm::vec3& v3) const {
    glUniform3fv(location, 1, glm::value_ptr(v3));
}

void GLProgram::setUniform(int location, const glm::vec4& v4) const {
    glUniform4fv(location, 1, glm::value_ptr(v4));
}

void GLProgram::setUniform(int location, const glm::mat2& mat2) const {
    glUniformMatrix2fv(location, 1, GL_FALSE, glm::value_ptr(mat2));
}

void GLProgram::setUniform(int location, const glm::mat3& mat3) const {
    glUniformMatrix3fv(location, 1, GL_FALSE, glm::value_ptr(mat3));
}

void GLProgram::setUniform(int location, const glm::mat4& mat4) const {
    glUniformMatrix4fv(location, 1, GL_FALSE, glm::value_ptr(mat4));
}

void GLProgram::printResourceInfos() const {
    std::cout << "Resource Infos" << std::endl;
    printUniformVarInfos();
    printTextureInfos();
    printUniformBlockInfos();
    printStorageBufferInfos();
    printStorageImageInfos();
    printAtomicCounterInfos();
}

void GLProgram::printUniformVarInfos() const {
    std::cout << "+ Uniform Var Infos" << std::endl;
    for (auto const& [name, varInfo] : m_uniformVarInfos) {
        std::cout << "  + " << name << std::endl;
        std::cout << "    + location: " << varInfo.location << std::endl;
        std::cout << "    + type:     " << magic_enum::enum_name(varInfo.type) << std::endl;
    }
}

void GLProgram::printTextureInfos() const {
    std::cout << "+ Texture Infos" << std::endl;
    for (auto const& [name, texInfo] : m_textureInfos) {
        std::cout << "  + " << name << std::endl;
        std::cout << "    + binding: " << texInfo.binding << std::endl;
        std::cout << "    + type:    " << magic_enum::enum_name(texInfo.type) << std::endl;
    }
}

void GLProgram::printUniformBlockInfos() const {
    std::cout << "+ Uniform Block Infos" << std::endl;
    for (auto const& [name, uboInfo] : m_uniformBlockInfos) {
        std::cout << "  + " << name << std::endl;
        std::cout << "    + binding: " << uboInfo.binding << std::endl;
        std::cout << "    + size:    " << uboInfo.size << std::endl;
    }
}

void GLProgram::printStorageBufferInfos() const {
    std::cout << "+ Storage Buffer Infos" << std::endl;
    for (auto const& [name, ssboInfo] : m_storageBufferInfos) {
        std::cout << "  + " << name << std::endl;
        std::cout << "    + binding: " << ssboInfo.binding << std::endl;
    }
}

void GLProgram::printStorageImageInfos() const {
    std::cout << "+ Storage Image Infos" << std::endl;
    for (auto const& [name, imageInfo] : m_storageImageInfos) {
        std::cout << "  + " << name << std::endl;
        std::cout << "    + binding: " << imageInfo.binding << std::endl;
        std::cout << "    + type:    " << magic_enum::enum_name(imageInfo.type) << std::endl;
    }
}

void GLProgram::printAtomicCounterInfos() const {
    std::cout << "+ Atomic Counter Infos" << std::endl;
    for (auto const& [name, atomicInfo] : m_atomicCounterInfos) {
        std::cout << "  + " << name << std::endl;
        std::cout << "    + binding:    " << atomicInfo.binding << std::endl;
        std::cout << "    + offset:  " << atomicInfo.offset << std::endl;
    }
}