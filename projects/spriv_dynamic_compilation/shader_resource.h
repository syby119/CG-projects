#pragma once
#include <cstdint>

/* note:
   for struct, it will be seperated to basic types
   for array, it will be seperated to basic elements
 */
enum class ShaderVarType {
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

struct ShaderUniformVarInfo {
    uint32_t location;
    ShaderVarType type;
};

struct ShaderUniformBlockInfo {
    uint32_t binding;
    uint32_t size;
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