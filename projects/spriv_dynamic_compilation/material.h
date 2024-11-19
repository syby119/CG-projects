#pragma once

#include <glm/glm.hpp>
#include <memory>
#include <map>
#include <variant>

#include "gl_program.h"
#include "../base/texture.h"

class Material {
public:
    Material(std::shared_ptr<GLProgram> program);

    void upload();

    template <typename T>
    void set(const std::string& name, T const& value) {
        auto attribute = m_program->getAttribute(name);
        if (attribute) {
            attribute->set(value);
        }
    }

    void set(const std::string& name, std::shared_ptr<Texture> texture) {
        //m_textures.push_back(texture);
    }

private:
    using ShaderUniform = std::variant<
        bool, glm::bvec2, glm::bvec3, glm::bvec4,
        int, glm::ivec2, glm::ivec3, glm::ivec4,
        unsigned, glm::uvec2, glm::uvec3, glm::uvec4,
        float, glm::vec2, glm::vec3, glm::vec4,
        glm::mat2, glm::mat2x3, glm::mat2x4,
        glm::mat3x2, glm::mat3, glm::mat3x4,
        glm::mat4x2, glm::mat4x3, glm::mat4>;

private:
    std::shared_ptr<GLProgram> m_program;

    std::map<std::string, ShaderUniform> m_attributes;

    std::map<std::string, std::shared_ptr<Texture>> m_textures;
};