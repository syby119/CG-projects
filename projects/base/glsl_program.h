#pragma once

#include <string>
#include <vector>

#include <glad/glad.h>
#include <glm/glm.hpp>

class GLSLProgram {
public:
    GLSLProgram();

    GLSLProgram(GLSLProgram&& rhs) noexcept;

    ~GLSLProgram();

    void attachVertexShader(const std::string& code);

    void attachGeometryShader(const std::string& filePath);

    void attachFragmentShader(const std::string& code);

    void attachVertexShaderFromFile(const std::string& filePath);

    void attachGeometryShaderFromFile(const std::string& filePath);

    void attachFragmentShaderFromFile(const std::string& filePath);

    void setTransformFeedbackVaryings(
        const std::vector<const char*>& varyings, GLenum bufferMode);

    void link();

    void use();

    int getUniformBlockSize(const std::string& name) const;

    int getUniformBlockIndex(const std::string& name) const;

    int getUniformBlockVariableOffset(const std::string& name) const;

    void setUniformBool(const std::string& name, bool value) const;

    void setUniformInt(const std::string& name, int value) const;

    void setUniformUint(const std::string& name, uint32_t value) const;

    void setUniformFloat(const std::string& name, float value) const;

    void setUniformVec2(const std::string& name, const glm::vec2& v2) const;

    void setUniformVec3(const std::string& name, const glm::vec3& v3) const;

    void setUniformVec4(const std::string& name, const glm::vec4& v4) const;

    void setUniformMat2(const std::string& name, const glm::mat2& mat2) const;

    void setUniformMat3(const std::string& name, const glm::mat3& mat3) const;

    void setUniformMat4(const std::string& name, const glm::mat4& mat4) const;

    void setUniformBlockBinding(const std::string& name, uint32_t binding) const;

public:
    GLuint _handle = 0;

    std::vector<GLuint> _vertexShaders;

    std::vector<GLuint> _geometryShaders;

    std::vector<GLuint> _fragmentShaders;

    std::string readFile(const std::string& filePath);

    GLuint createShader(const std::string& code, GLenum shaderType);
};