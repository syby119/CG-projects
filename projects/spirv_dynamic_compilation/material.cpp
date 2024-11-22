#include "material.h"
#include "../base/texture2d.h"
#include "../base/texture_cubemap.h"

Material::Material(std::shared_ptr<GLProgram> program) : m_program{ program } {
    for (auto const& [fullname, varInfo] : m_program->getUniformVarInfos()) {
        if (fullname.starts_with(m_materialFieldPrefix)) {
            std::string name;
            bool isColor{ false };
            if (fullname.starts_with(m_materialFieldColorPrefix)) {
                name = fullname.substr(m_materialFieldColorPrefix.size());
                isColor = true;
            }
            else {
                name = fullname.substr(m_materialFieldPrefix.size());
            }

            switch (varInfo.type) {
            case GLProgram::VarType::Bool:
                m_attributeInfos[name] = { varInfo.location, varInfo.type, isColor , false };
                break;
            case GLProgram::VarType::BVec2:
                m_attributeInfos[name] = { varInfo.location, varInfo.type, isColor , glm::bvec2{ false } };
                break;
            case GLProgram::VarType::BVec3:
                m_attributeInfos[name] = { varInfo.location, varInfo.type, isColor , glm::bvec3{ false } };
                break;
            case GLProgram::VarType::BVec4:
                m_attributeInfos[name] = { varInfo.location, varInfo.type, isColor , glm::bvec4{ false } };
                break;
            case GLProgram::VarType::Int:
                m_attributeInfos[name] = { varInfo.location, varInfo.type, isColor , 0 };
                break;
            case GLProgram::VarType::IVec2:
                m_attributeInfos[name] = { varInfo.location, varInfo.type, isColor , glm::ivec2{ 0 } };
                break;
            case GLProgram::VarType::IVec3:
                m_attributeInfos[name] = { varInfo.location, varInfo.type, isColor , glm::ivec3{ 0 } };
                break;
            case GLProgram::VarType::IVec4:
                m_attributeInfos[name] = { varInfo.location, varInfo.type, isColor , glm::ivec4{ 0 } };
                break;
            case GLProgram::VarType::UInt:
                m_attributeInfos[name] = { varInfo.location, varInfo.type, isColor , 0u };
                break;
            case GLProgram::VarType::UVec2:
                m_attributeInfos[name] = { varInfo.location, varInfo.type, isColor , glm::uvec2{ 0u } };
                break;
            case GLProgram::VarType::UVec3:
                m_attributeInfos[name] = { varInfo.location, varInfo.type, isColor , glm::uvec3{ 0u } };
                break;
            case GLProgram::VarType::UVec4:
                m_attributeInfos[name] = { varInfo.location, varInfo.type, isColor , glm::uvec4{ 0u } };
                break;
            case GLProgram::VarType::Float:
                m_attributeInfos[name] = { varInfo.location, varInfo.type, isColor , 0.0f };
                break;
            case GLProgram::VarType::Vec2:
                m_attributeInfos[name] = { varInfo.location, varInfo.type, isColor , glm::vec2{ 0.0f } };
                break;
            case GLProgram::VarType::Vec3:
                m_attributeInfos[name] = { varInfo.location, varInfo.type, isColor , glm::vec3{ 0.0f } };
                break;
            case GLProgram::VarType::Vec4:
                m_attributeInfos[name] = { varInfo.location, varInfo.type, isColor , glm::vec4{ 0.0f } };
                break;
            case GLProgram::VarType::Double:
                m_attributeInfos[name] = { varInfo.location, varInfo.type, isColor , 0.0 };
                break;
            case GLProgram::VarType::DVec2:
                m_attributeInfos[name] = { varInfo.location, varInfo.type, isColor , glm::dvec2{ 0.0 } };
                break;
            case GLProgram::VarType::DVec3:
                m_attributeInfos[name] = { varInfo.location, varInfo.type, isColor , glm::dvec3{ 0.0 } };
                break;
            case GLProgram::VarType::DVec4:
                m_attributeInfos[name] = { varInfo.location, varInfo.type, isColor , glm::dvec4{ 0.0 } };
                break;
            case GLProgram::VarType::Mat2x2:
                m_attributeInfos[name] = { varInfo.location, varInfo.type, isColor , glm::mat2x2(0.0f) };
                break;
            case GLProgram::VarType::Mat2x3:
                m_attributeInfos[name] = { varInfo.location, varInfo.type, isColor , glm::mat2x3(0.0f) };
                break;
            case GLProgram::VarType::Mat2x4:
                m_attributeInfos[name] = { varInfo.location, varInfo.type, isColor , glm::mat2x4(0.0f) };
                break;
            case GLProgram::VarType::Mat3x2:
                m_attributeInfos[name] = { varInfo.location, varInfo.type, isColor , glm::mat3x2(0.0f) };
                break;
            case GLProgram::VarType::Mat3x3:
                m_attributeInfos[name] = { varInfo.location, varInfo.type, isColor , glm::mat3x3(0.0f) };
                break;
            case GLProgram::VarType::Mat3x4:
                m_attributeInfos[name] = { varInfo.location, varInfo.type, isColor , glm::mat3x4(0.0f) };
                break;
            case GLProgram::VarType::Mat4x2:
                m_attributeInfos[name] = { varInfo.location, varInfo.type, isColor , glm::mat4x2(0.0f) };
                break;
            case GLProgram::VarType::Mat4x3:
                m_attributeInfos[name] = { varInfo.location, varInfo.type, isColor , glm::mat4x3(0.0f) };
                break;
            case GLProgram::VarType::Mat4x4:
                m_attributeInfos[name] = { varInfo.location, varInfo.type, isColor , glm::mat4x4(0.0f) };
                break;
            case GLProgram::VarType::DMat2x2:
                m_attributeInfos[name] = { varInfo.location, varInfo.type, isColor , glm::dmat2x2(0.0) };
                break;
            case GLProgram::VarType::DMat2x3:
                m_attributeInfos[name] = { varInfo.location, varInfo.type, isColor , glm::dmat2x2(0.0) };
                break;
            case GLProgram::VarType::DMat2x4:
                m_attributeInfos[name] = { varInfo.location, varInfo.type, isColor , glm::dmat2x2(0.0) };
                break;
            case GLProgram::VarType::DMat3x2:
                m_attributeInfos[name] = { varInfo.location, varInfo.type, isColor , glm::dmat2x2(0.0) };
                break;
            case GLProgram::VarType::DMat3x3:
                m_attributeInfos[name] = { varInfo.location, varInfo.type, isColor , glm::dmat2x2(0.0) };
                break;
            case GLProgram::VarType::DMat3x4:
                m_attributeInfos[name] = { varInfo.location, varInfo.type, isColor , glm::dmat2x2(0.0) };
                break;
            case GLProgram::VarType::DMat4x2:
                m_attributeInfos[name] = { varInfo.location, varInfo.type, isColor , glm::dmat2x2(0.0) };
                break;
            case GLProgram::VarType::DMat4x3:
                m_attributeInfos[name] = { varInfo.location, varInfo.type, isColor , glm::dmat2x2(0.0) };
                break;
            case GLProgram::VarType::DMat4x4:
                m_attributeInfos[name] = { varInfo.location, varInfo.type, isColor , glm::dmat2x2(0.0) };
                break;
            }
        }
    }

    for (auto const& [name, texInfo] : m_program->getTextureInfos()) {
        if (name.starts_with(m_materialFieldPrefix)) {
            std::string truncated{ name.substr(m_materialFieldPrefix.size()) };
            m_textureInfos[truncated] = { texInfo.binding, texInfo.type, nullptr };
        }
    }
}

void Material::upload() {
    for (auto const& [name, attributeInfo] : m_attributeInfos) {
        std::visit([&](auto&& attr) {
            m_program->setUniform(attributeInfo.location, attr);
            }, attributeInfo.value);
    }

    for (auto const& [name, texInfo] : m_textureInfos) {
        if (texInfo.texture) {
            texInfo.texture->bind(texInfo.binding);
        }
    }
}

bool Material::set(const std::string& name, std::shared_ptr<Texture> texture) {
    if (auto it = m_textureInfos.find(name); it != m_textureInfos.end()) {
        if (!texture) {
            m_textureInfos[name].texture = nullptr;
            return true;
        }

        switch (it->second.type) {
        case GLProgram::VarType::Tex2D:
            if (std::dynamic_pointer_cast<Texture2D>(texture)) {
                m_textureInfos[name].texture = texture;
                return true;
            }
            break;
        case GLProgram::VarType::Tex2DArray:
            if (std::dynamic_pointer_cast<Texture2DArray>(texture)) {
                m_textureInfos[name].texture = texture;
                return true;
            }
            break;
        case GLProgram::VarType::TexCube:
            if (std::dynamic_pointer_cast<TextureCubemap>(texture)) {
                m_textureInfos[name].texture = texture;
                return true;
            }
            break;
        }
    }

    return false;
}