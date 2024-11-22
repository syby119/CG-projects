#pragma once

#include <map>
#include <memory>
#include <type_traits>
#include <variant>

#include <glm/glm.hpp>

#include "gl_program.h"
#include "../base/texture.h"

class Material {
public:
    using Attribute = std::variant<
        bool, glm::bvec2, glm::bvec3, glm::bvec4,
        int, glm::ivec2, glm::ivec3, glm::ivec4,
        unsigned, glm::uvec2, glm::uvec3, glm::uvec4,
        float, glm::vec2, glm::vec3, glm::vec4,
        double, glm::dvec2, glm::dvec3, glm::dvec4,
        glm::mat2x2, glm::mat2x3, glm::mat2x4,
        glm::mat3x2, glm::mat3x3, glm::mat3x4,
        glm::mat4x2, glm::mat4x3, glm::mat4x4,
        glm::dmat2x2, glm::dmat2x3, glm::dmat2x4,
        glm::dmat3x2, glm::dmat3x3, glm::dmat3x4,
        glm::dmat4x2, glm::dmat4x3, glm::dmat4x4>;

    template <typename T, typename Variant>
    struct isAttribute : std::false_type {};

    template <typename T, typename ... Types>
    struct isAttribute<T, std::variant<Types...>>
        : std::disjunction<std::is_same<std::decay_t<T>, Types>...> {};

    struct AttributeInfo {
        int location;
        GLProgram::VarType type;
        bool isColor;
        Attribute value;
    };

    struct TextureInfo {
        int binding;
        GLProgram::VarType type;
        std::shared_ptr<Texture> texture;
    };

public:
    Material(std::shared_ptr<GLProgram> program);

    std::shared_ptr<GLProgram> getProgram() const { return m_program; }

    void upload();

    template <typename T, typename = std::enable_if_t<isAttribute<T, Attribute>::value>>
    bool set(std::string const& name, T const& value) {
        if (auto it = m_attributeInfos.find(name); it != m_attributeInfos.end()) {
            if (std::holds_alternative<T>(it->second.value)) {
                it->second.value = value;
                return true;
            }
        }

        return false;
    }

    bool set(std::string const& name, std::shared_ptr<Texture> texture);

    std::map<std::string, AttributeInfo> const& getArributeInfos() const noexcept { return m_attributeInfos; }

    std::map<std::string, TextureInfo> const& getTextureInfos() const noexcept { return m_textureInfos; }

private:
    std::shared_ptr<GLProgram> m_program;

    std::map<std::string, AttributeInfo> m_attributeInfos;

    std::map<std::string, TextureInfo> m_textureInfos;

private:
    static constexpr std::string_view m_materialFieldPrefix{ "_MATERIAL_FIELD_" };
    static constexpr std::string_view m_materialFieldColorPrefix{ "_MATERIAL_FIELD_COLOR_" };
};