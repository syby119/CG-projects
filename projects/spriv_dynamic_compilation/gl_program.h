#pragma once

#include "../base/gl_utility.h"
#include <glm/glm.hpp>

#include <map>
#include <string_view>
#include <vector>

#include "shader_module.h"

class GLProgram {
public:
    /* note:
       for struct, it will be seperated to basic types
       for array, it will be seperated to basic elements
     */
    enum class VarType {
        Unknown,
        Bool,
        BVec2,
        BVec3,
        BVec4,
        Int,
        IVec2,
        IVec3,
        IVec4,
        UInt,
        UVec2,
        UVec3,
        UVec4,
        Float,
        Vec2,
        Vec3,
        Vec4,
        Double,
        DVec2,
        DVec3,
        DVec4,
        Mat2x2,
        Mat2x3,
        Mat2x4,
        Mat3x2,
        Mat3x3,
        Mat3x4,
        Mat4x2,
        Mat4x3,
        Mat4x4,
        DMat2x2,
        DMat2x3,
        DMat2x4,
        DMat3x2,
        DMat3x3,
        DMat3x4,
        DMat4x2,
        DMat4x3,
        DMat4x4,
        Tex1D,
        Tex2D,
        Tex3D,
        TexCube,
        Tex2DRect,
        Tex1DArray,
        Tex2DArray,
        TexCubeArray,
        TexBuffer,
        Tex2DMS,
        Tex2DMSArray,
        Image1D,
        Image2D,
        Image3D,
        ImageCube,
        Image2DRect,
        Image1DArray,
        Image2DArray,
        ImageCubeArray,
        ImageBuffer,
        Image2DMS,
        Image2DMSArray,
        StorageBuffer,
        AtomicCounter,
        // Not in OpenGL
        //AccelerationStructure,
        //RayQuery
    };

    struct UniformVarInfo {
        VarType type;
        int location;
    };

    struct TextureInfo {
        int binding;
        VarType type;
    };

    struct UniformBlockInfo {
        int binding;
        uint32_t size;
    };

    struct StorageBufferInfo {
        int binding;
    };

    struct StorageImageInfo {
        int binding;
        VarType type;
    };

    struct AtomicCounterInfo {
        int binding;
        int offset;
    };

    using UniformVarInfoMap = std::map<std::string, UniformVarInfo>;

    using TextureInfoMap = std::map<std::string, TextureInfo>;

    using UniformBlockInfoMap = std::map<std::string, UniformBlockInfo>;

    using StorageBufferInfoMap = std::map<std::string, StorageBufferInfo>;

    using StorageImageInfoMap = std::map<std::string, StorageImageInfo>;

    using AtomicCounterInfoMap = std::map<std::string, AtomicCounterInfo>;

public:
    GLProgram();

    GLProgram(GLProgram&& rhs) noexcept;

    ~GLProgram();

    GLProgram& operator=(GLProgram&& rhs) noexcept;

    void use();

    void unuse();

    int getUniformVarLocation(std::string_view name) const;

    int getTextureBinding(std::string_view name) const;

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

    void printResourceInfos() const;

    void printUniformVarInfos() const;

    void printTextureInfos() const;

    void printUniformBlockInfos() const;

    void printStorageBufferInfos() const;

    void printStorageImageInfos() const;

    void printAtomicCounterInfos() const;

private:
    GLuint m_handle{ 0 };

    UniformVarInfoMap m_uniformVarInfos;
    TextureInfoMap m_textureInfos;
    UniformBlockInfoMap m_uniformBlockInfos;
    StorageBufferInfoMap m_storageBufferInfos;
    StorageImageInfoMap m_storageImageInfos;
    AtomicCounterInfoMap m_atomicCounterInfos;

private:
    void attach(ShaderModule const& shaderModule);

    void detach(ShaderModule const& shaderModule);

    void link();

private:
    friend class ProgramManager;
};