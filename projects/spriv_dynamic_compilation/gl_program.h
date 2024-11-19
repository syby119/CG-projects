#pragma once

#include "../base/gl_utility.h"
#include <glm/glm.hpp>

#include <string_view>
#include <map>

#include "shader_module.h"
#include "shader_resource.h"

class GLProgram {
public:
    using ShaderUniformVarInfoMap = std::map<std::string, ShaderUniformVarInfo>;

public:
    GLProgram();

    GLProgram(GLProgram&& rhs) noexcept;

    ~GLProgram();

    GLProgram& operator=(GLProgram&& rhs) noexcept;

    void attach(ShaderModule const& shaderModule);

    void detach(ShaderModule const& shaderModule);

    void link();

    void setUniformInfoMap(ShaderUniformVarInfoMap const& uniformInfoMap);

    void use();

    void unuse();

    int getUniformLocation(std::string_view name) const;

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

    std::map<std::string, ShaderUniformVarInfo> m_uniformInfoMap;
};