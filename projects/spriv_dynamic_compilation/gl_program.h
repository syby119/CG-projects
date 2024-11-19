#pragma once

#include "../base/gl_utility.h"
#include <glm/glm.hpp>

#include <string>
#include <vector>

#include "shader_module.h"


class GLProgram {
public:
    GLProgram();

    GLProgram(GLProgram&& rhs) noexcept;

    ~GLProgram();

    GLProgram& operator=(GLProgram&& rhs) noexcept;

    void attach(ShaderModule const& shaderModule);

    void detach(ShaderModule const& shaderModule);

    void link();

    void use();

    void unuse();

    void setUniform(int location, bool value) const;

    void setUniform(int location, int value) const;

    void setUniform(int location, uint32_t value) const;

    void setUniform(int location, float value) const;

    void setUniform(int location, const glm::vec2& v2) const;

    void setUniform(int location, const glm::vec3& v3) const;

    void setUniform(int location, const glm::vec4& v4) const;

    void setUniform(int location, const glm::mat2& mat2) const;

    void setUniform(int location, const glm::mat3& mat3) const;

    void setUniform(int location, const glm::mat4& mat4) const;

private:
    GLuint m_handle{ 0 };
};