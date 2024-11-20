#pragma once

#include "../base/gl_utility.h"
#include <glm/glm.hpp>

#include <map>
#include <string_view>
#include <vector>

#include "shader_module.h"
#include "shader_resource.h"

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
        Mat2,
        Mat2x3,
        Mat2x4,
        Mat3x2,
        Mat3,
        Mat3x4,
        Mat4x2,
        Mat4x3,
        Mat4,
        DMat2,
        DMat2x3,
        DMat2x4,
        DMat3x2,
        DMat3,
        DMat3x4,
        DMat4x2,
        DMat4x3,
        DMat4,
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
        AccelerationStructure,
        RayQuery
    };

    using BindingPoint = int;

    struct UniformVarInfo {
        VarType type;
        int location;
    };

    struct UniformBlockVarInfo {
        VarType type;
        int offset;
    };

    struct AtomicCounterVarInfo {
        std::string name;
        int offset;
    };

    struct UniformBlockInfo {
        int binding;
        uint32_t size;
        std::map<std::string, UniformBlockVarInfo> varInfoMap;
    };

    struct ShaderStorageBufferInfo {
        BindingPoint binding;
    };

    struct TextureInfo {
        BindingPoint binding;
        VarType type;
    };

    struct ImageInfo {
        BindingPoint binding;
        VarType type;
    };

    struct AtomicCounterVarInfo {
        std::string name;
        int offset;
    };

    struct AtomicCounterBlockVarInfo {
        uint32_t size;
        std::vector<AtomicCounterVarInfo> varInfos;
    };

    using UniformVarInfoMap = std::map<std::string, ShaderUniformVarInfo>;

    using UniformBlockInfoMap = std::map<std::string, UniformBlockInfo>;

    using ShaderStorageBufferInfoMap = std::map<std::string, ShaderStorageBufferInfo>;

    using ImageInfoMap = std::map<std::string, ImageInfo>;

    using AtomicCounterBufferInfoMap = std::map<BindingPoint, AtomicCounterBlockVarInfo>;

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

    UniformVarInfoMap m_uniformVarInfos;
    UniformBlockInfoMap m_uniformBlockInfos;
    ShaderStorageBufferInfoMap m_storageBufferInfos;
    ImageInfoMap m_imageInfoMap;
    AtomicCounterBufferInfoMap m_atomicCounterInfoMap;

    friend class ProgramManager;
};