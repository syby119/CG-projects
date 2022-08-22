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

    void attachFragmentShader(const std::string& code);

    void attachVertexShaderFromFile(const std::string& filePath);

    void attachFragmentShaderFromFile(const std::string& filePath);

    void setTransformFeedbackVaryings(
        const std::vector<const char*>& varyings, GLenum bufferMode);

    void link();

    void use();

    void setBool(const std::string& name, bool value) const;

    void setInt(const std::string& name, int value) const;

    void setFloat(const std::string& name, float value) const;

    void setVec2(const std::string& name, const glm::vec2& v2) const;

    void setVec3(const std::string& name, const glm::vec3& v3) const;

    void setVec4(const std::string& name, const glm::vec4& v4) const;

    void setMat3(const std::string& name, const glm::mat3& mat3) const;

    void setMat4(const std::string& name, const glm::mat4& mat4) const;

public:
    GLuint _handle = 0;

    std::vector<GLuint> _vertexShaders;

    std::vector<GLuint> _fragmentShaders;

    std::string readFile(const std::string& filePath);

    GLuint createShader(const std::string& code, GLenum shaderType);
};