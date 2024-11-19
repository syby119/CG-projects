#pragma once
#include <cstdint>

struct ShaderVarType {
    /* note:
       for array, it will be seperated to basic types
       for struct, it will be seperated to basic types
     */
    enum Type {
        Bool, BVec2, BVec3, BVec4,
        Int, IVec2, IVec3, IVec4,
        UInt, UInt2, UInt3, UInt4,
        Float, Vec2, Vec3, Vec4,
        Mat2, Mat2x3, Mat2x4,
        Mat3x2, Mat3, Mat3x4,
        Mat4x2, Mat4x3, Mat4,
        Tex2D, Tex3D, TexCube,
        Image2D, Image3D, ImageCube,
        StorageBuffer,
    };
};

struct ShaderUniformVarInfo {
    uint32_t location;
    ShaderVarType type;
};

struct ShaderStorgaeBufferInfo {
    uint32_t binding;
};

struct ShaderImageInfo {
    uint32_t binding;
};

struct ShaderTextureInfo {
    uint32_t binding;
};