#include "program_manager.h"

#include <shaderc/shaderc.hpp>
#include <spirv_cross.hpp>
#include <spirv_glsl.hpp>
#include <fstream>
#include <magic_enum/magic_enum.hpp>

namespace details {
    class ShaderIncluder : public shaderc::CompileOptions::IncluderInterface {
    public:
        ShaderIncluder(ProgramManager& pm) : m_programManager{ pm } {}

        shaderc_include_result* GetInclude(
            const char* includeFilepath,
            shaderc_include_type type,
            const char* shaderFilepath,
            size_t includeDepth) override {
            std::filesystem::path includeFullPath;
            // search in the relative directory
            if (type == shaderc_include_type_relative) {
                auto shaderDir{ std::filesystem::path(shaderFilepath).parent_path() };
                auto searchPath{ shaderDir / includeFilepath };
                std::error_code errCode;
                if (std::filesystem::exists(searchPath, errCode)) {
                    includeFullPath = searchPath;
                }
            }

            // search in the system directory
            if (includeFullPath.empty()) {
                for (auto const& directory : m_programManager.getIncludeDirectories()) {
                    auto searchPath{ directory / includeFilepath };
                    std::error_code errCode;
                    if (std::filesystem::exists(searchPath, errCode)) {
                        includeFullPath = searchPath;
                        break;
                    }
                }
            }

            bool success{ !includeFullPath.empty() };

            // make sure to absolute path
            if (success) {
                std::error_code errCode;
                includeFullPath = std::filesystem::absolute(includeFullPath, errCode);
                if (errCode) {
                    includeFullPath.clear();
                    success = false;
                }
            }

            // read file content
            std::string contents;
            if (success) {
                success = ProgramManager::readFile(includeFullPath, contents);
                if (!success) {
                    contents = "Read File Failure";
                }
            }
            else {
                contents = "Cannot Open File";
            }

            auto persistentContainer{ new std::string[2] };
            persistentContainer[0] = includeFullPath.string();
            persistentContainer[1] = contents;

            auto shaderIncludeResult{ new shaderc_include_result };
            shaderIncludeResult->source_name = persistentContainer[0].data();
            shaderIncludeResult->source_name_length = persistentContainer[0].size();
            shaderIncludeResult->content = persistentContainer[1].data();
            shaderIncludeResult->content_length = persistentContainer[1].size();
            shaderIncludeResult->user_data = persistentContainer;

            return shaderIncludeResult;
        }

        void ReleaseInclude(shaderc_include_result* data) override {
            delete[] reinterpret_cast<std::string*>(data->user_data);
            delete data;
        }

    private:
        ProgramManager& m_programManager;
    };

    static std::string getTypeStr(spirv_cross::SPIRType type) {
        switch (type.basetype) {
        case spirv_cross::SPIRType::Unknown: return "????";
        case spirv_cross::SPIRType::Void: return "void";
        case spirv_cross::SPIRType::Boolean: return "bool";
        case spirv_cross::SPIRType::SByte: return "int8";
        case spirv_cross::SPIRType::UByte: return "uint8";
        case spirv_cross::SPIRType::Short: return "int16";
        case spirv_cross::SPIRType::UShort: return "uint16";
        case spirv_cross::SPIRType::Int: return "int";
        case spirv_cross::SPIRType::UInt: return "uint";
        case spirv_cross::SPIRType::Int64: return "int64";
        case spirv_cross::SPIRType::UInt64: return "uint64";
        case spirv_cross::SPIRType::AtomicCounter: return "atomic";
        case spirv_cross::SPIRType::Half: return "half";
        case spirv_cross::SPIRType::Float: return "float";
        case spirv_cross::SPIRType::Double: return "double";
        case spirv_cross::SPIRType::Struct: return "struct";
        case spirv_cross::SPIRType::Image: return "image";
        case spirv_cross::SPIRType::SampledImage: return "gsampler";
        case spirv_cross::SPIRType::Sampler: return "sampler";
        case spirv_cross::SPIRType::AccelerationStructure: return "AccelerationStructure";
        case spirv_cross::SPIRType::RayQuery: return "RayQuery";
        case spirv_cross::SPIRType::ControlPointArray: return "ControlPointArray";
        case spirv_cross::SPIRType::Interpolant: return "Interpolant";
        case spirv_cross::SPIRType::Char: return "Char";
        case spirv_cross::SPIRType::MeshGridProperties: return "MeshGridProperties";
        }

        return "Unknown";
    }

    static bool getNextIndices(
        spirv_cross::SmallVector<uint32_t> const& limits, std::vector<uint32_t>& indices) {
        // @ref: https://github.com/KhronosGroup/SPIRV-Cross/wiki/Reflection-API-user-guide
        // spirv_cross treat the array of array
        // int a[4][6];
        // limits = { 6, 4 };
        for (int i = 0; i < indices.size(); ++i) {
            if (indices[i] + 1 >= limits[i]) {
                indices[i] = 0;
            }
            else {
                indices[i] += 1;
                return true;
            }
        }

        return false;
    }

    static std::string getArrayIndexStr(std::vector<uint32_t> const& indices) {
        std::string indexStr;
        for (auto it = indices.rbegin(); it != indices.rend(); ++it) {
            indexStr += "[" + std::to_string(*it) + "]";
        }
        return indexStr;
    }

    static GLProgram::VarType getImageType(spirv_cross::SPIRType const& type) {
        if (type.basetype == spirv_cross::SPIRType::SampledImage) {
            if (type.image.sampled != 1) {
                return GLProgram::VarType::Unknown;
            }

            switch (type.image.dim) {
            case spv::Dim1D:
                if (!type.image.arrayed) {
                    return GLProgram::VarType::Tex1D;
                }
                else {
                    return GLProgram::VarType::Tex1DArray;
                }
            case spv::Dim2D:
                if (type.image.ms) {
                    if (!type.image.arrayed) {
                        return GLProgram::VarType::Tex2DMS;
                    }
                    else {
                        return GLProgram::VarType::Tex2DMSArray;
                    }
                }
                else {
                    if (!type.image.arrayed) {
                        return GLProgram::VarType::Tex2D;
                    }
                    else {
                        return GLProgram::VarType::Tex2DArray;
                    }
                }
            case spv::Dim3D: return GLProgram::VarType::Tex3D;
            case spv::DimCube:
                if (!type.image.arrayed) {
                    return GLProgram::VarType::TexCube;
                }
                else {
                    return GLProgram::VarType::TexCubeArray;
                }
            case spv::DimRect: return GLProgram::VarType::Tex2DRect;
            case spv::DimBuffer: return GLProgram::VarType::TexBuffer;
            }
        }
        else if (type.basetype == spirv_cross::SPIRType::Image) {
            if (type.image.sampled != 2) {
                return GLProgram::VarType::Unknown;
            }

            switch (type.image.dim) {
            case spv::Dim1D:
                if (!type.image.arrayed) {
                    return GLProgram::VarType::Image1D;
                }
                else {
                    return GLProgram::VarType::Image1DArray;
                }
            case spv::Dim2D:
                if (type.image.ms) {
                    if (!type.image.arrayed) {
                        return GLProgram::VarType::Image2DMS;
                    }
                    else {
                        return GLProgram::VarType::Image2DMSArray;
                    }
                }
                else {
                    if (!type.image.arrayed) {
                        return GLProgram::VarType::Image2D;
                    }
                    else {
                        return GLProgram::VarType::Image2DArray;
                    }
                }
            case spv::Dim3D: return GLProgram::VarType::Image3D;
            case spv::DimCube:
                if (!type.image.arrayed) {
                    return GLProgram::VarType::ImageCube;
                }
                else {
                    return GLProgram::VarType::ImageCubeArray;
                }
            case spv::DimRect: return GLProgram::VarType::Image2DRect;
            case spv::DimBuffer: return GLProgram::VarType::ImageBuffer;
            }
        }

        return GLProgram::VarType::Unknown;
    }

    static int reflectUniformVarRecursive(
        spirv_cross::Compiler const& compiler,
        spirv_cross::SPIRType const& type,
        std::string const& name, int location,
        GLProgram::UniformVarInfoMap& uniformVarInfos) {
        using VarType = GLProgram::VarType;
        using UniformVarInfo = GLProgram::UniformVarInfo;

        VarType varType{ VarType::Unknown };
        // non struct case
        if (type.basetype != spirv_cross::SPIRType::Struct) {
            if (type.vecsize == 1) {
                switch (type.basetype) {
                case spirv_cross::SPIRType::Boolean: varType = VarType::Bool; break;
                case spirv_cross::SPIRType::Int: varType = VarType::Int; break;
                case spirv_cross::SPIRType::UInt: varType = VarType::UInt; break;
                case spirv_cross::SPIRType::Float: varType = VarType::Float; break;
                case spirv_cross::SPIRType::Double: varType = VarType::Double; break;
                case spirv_cross::SPIRType::SampledImage: varType = details::getImageType(type); break;
                }
            }
            else {
                // can be a vector/matrix
                if (type.columns == 1) {
                    switch (type.vecsize) {
                    case 2:
                        switch (type.basetype) {
                        case spirv_cross::SPIRType::Boolean: varType = VarType::BVec2; break;
                        case spirv_cross::SPIRType::Int: varType = VarType::IVec2; break;
                        case spirv_cross::SPIRType::UInt: varType = VarType::UVec2; break;
                        case spirv_cross::SPIRType::Float: varType = VarType::Vec2; break;
                        case spirv_cross::SPIRType::Double: varType = VarType::DVec2; break;
                        }
                        break;
                    case 3:
                        switch (type.basetype) {
                        case spirv_cross::SPIRType::Boolean: varType = VarType::BVec3; break;
                        case spirv_cross::SPIRType::Int: varType = VarType::IVec3; break;
                        case spirv_cross::SPIRType::UInt: varType = VarType::UVec3; break;
                        case spirv_cross::SPIRType::Float: varType = VarType::Vec3; break;
                        case spirv_cross::SPIRType::Double: varType = VarType::DVec3; break;
                        }
                        break;
                    case 4:
                        switch (type.basetype) {
                        case spirv_cross::SPIRType::Boolean: varType = VarType::BVec4; break;
                        case spirv_cross::SPIRType::Int: varType = VarType::IVec4; break;
                        case spirv_cross::SPIRType::UInt: varType = VarType::UVec4; break;
                        case spirv_cross::SPIRType::Float: varType = VarType::Vec4; break;
                        case spirv_cross::SPIRType::Double: varType = VarType::DVec4; break;
                        }
                        break;
                    }
                }
                else if (type.columns > 1) {
                    // can be a matrix
                    if (type.basetype == spirv_cross::SPIRType::Float) {
                        switch (type.vecsize) {
                        case 2:
                            switch (type.columns) {
                            case 2: varType = VarType::Mat2x2; break;
                            case 3: varType = VarType::Mat3x2; break;
                            case 4: varType = VarType::Mat4x2; break;
                            }
                            break;
                        case 3:
                            switch (type.columns) {
                            case 2: varType = VarType::Mat2x3; break;
                            case 3: varType = VarType::Mat3x3; break;
                            case 4: varType = VarType::Mat4x3; break;
                            }
                            break;
                        case 4:
                            switch (type.columns) {
                            case 2: varType = VarType::Mat2x4; break;
                            case 3: varType = VarType::Mat3x4; break;
                            case 4: varType = VarType::Mat4x4; break;
                            }
                            break;
                        }
                    }
                    else if (type.basetype == spirv_cross::SPIRType::Double) {
                        switch (type.vecsize) {
                        case 2:
                            switch (type.columns) {
                            case 2: varType = VarType::DMat2x2; break;
                            case 3: varType = VarType::DMat3x2; break;
                            case 4: varType = VarType::DMat4x2; break;
                            }
                            break;
                        case 3:
                            switch (type.columns) {
                            case 2: varType = VarType::DMat2x3; break;
                            case 3: varType = VarType::DMat3x3; break;
                            case 4: varType = VarType::DMat4x3; break;
                            }
                            break;
                        case 4:
                            switch (type.columns) {
                            case 2: varType = VarType::DMat2x4; break;
                            case 3: varType = VarType::DMat3x4; break;
                            case 4: varType = VarType::DMat4x4; break;
                            }
                            break;
                        }
                    }
                }
            }

            if (type.array.empty()) {
                uniformVarInfos[name] = { varType, location++ };
            }
            else {
                // handle array / array of array
                std::vector<uint32_t> indices(type.array.size(), 0);
                do {
                    auto nameIndexed = name + details::getArrayIndexStr(indices);
                    uniformVarInfos[nameIndexed] = { varType, location++ };
                } while (details::getNextIndices(type.array, indices));
            }

            return location;
        }

        // struct case
        if (type.array.empty()) {
            for (size_t i = 0; i < type.member_types.size(); ++i) {
                const auto& memberType{ compiler.get_type(type.member_types[i]) };
                auto memberName{ compiler.get_member_name(type.self, i) };
                if (memberName.empty()) {
                    std::cerr << "Shader uniform doesn't have name, fallback..." << std::endl;
                    memberName = compiler.get_fallback_member_name(i);
                }

                location = details::reflectUniformVarRecursive(
                    compiler, memberType,
                    name + "." + memberName, location, uniformVarInfos);
            }
        }
        else {
            // handle array / array of array
            std::vector<uint32_t> indices(type.array.size(), 0);
            do {
                auto nameIndexed = name + details::getArrayIndexStr(indices);
                for (size_t i = 0; i < type.member_types.size(); ++i) {
                    const auto& memberType{ compiler.get_type(type.member_types[i]) };
                    auto memberName{ compiler.get_member_name(type.self, i) };
                    if (memberName.empty()) {
                        std::cerr << "Shader uniform doesn't have name, fallback..." << std::endl;
                        memberName = compiler.get_fallback_member_name(i);
                    }

                    location = details::reflectUniformVarRecursive(
                        compiler, memberType,
                        nameIndexed + "." + memberName, location, uniformVarInfos);
                }
            } while (details::getNextIndices(type.array, indices));
        }

        return location;
    }

    static GLProgram::UniformVarInfoMap reflectUniformVarInfos(
        spirv_cross::Compiler const& compiler,
        spirv_cross::SmallVector<spirv_cross::Resource> const& uniforms) {
        // TODO: The spriv_cross failed to reflect top level array
        // https://github.com/KhronosGroup/SPIRV-Cross/issues/2421
        GLProgram::UniformVarInfoMap uniformVarInfos;
        for (auto const& uniform : uniforms) {
            // name
            std::string name{ uniform.name };
            if (name.empty()) {
                std::cerr << "Shader uniform doesn't have name, fallback..." << std::endl;
                name = compiler.get_fallback_name(uniform.id);
            }

            // location
            int location{ -1 };
            spirv_cross::Bitset mask{ compiler.get_decoration_bitset(uniform.id) };
            if (mask.get(spv::DecorationLocation)) {
                location = compiler.get_decoration(uniform.id, spv::DecorationLocation);
            }
            else {
                std::cerr << "Cannot get shader uniform location" << std::endl;
            }

            reflectUniformVarRecursive(
                compiler, compiler.get_type(uniform.type_id), name, location, uniformVarInfos);
        }

        return uniformVarInfos;
    }

    static GLProgram::TextureInfoMap reflectTextureInfos(
        spirv_cross::Compiler const& compiler,
        spirv_cross::SmallVector<spirv_cross::Resource> const& sampledImages) {
        using VarType = GLProgram::VarType;
        using TextureInfoMap = GLProgram::TextureInfoMap;

        TextureInfoMap textureInfos;
        for (auto const& sampledImage : sampledImages) {
            // name
            std::string name{ sampledImage.name };
            if (name.empty()) {
                std::cerr << "Shader texture doesn't have name, fallback..." << std::endl;
                name = compiler.get_fallback_name(spirv_cross::ID(sampledImage.id));
            }

            // binding
            int binding{ 0 };
            spirv_cross::Bitset mask{ compiler.get_decoration_bitset(sampledImage.id) };
            if (mask.get(spv::DecorationBinding)) {
                binding = compiler.get_decoration(sampledImage.id, spv::DecorationBinding);
            }
            else {
                std::cerr << "Cannot get texture binding" << std::endl;
                std::cerr << "Should vulkan workflow to specify texture binding, instead of location" << std::endl;
            }

            // type
            auto type{ compiler.get_type(sampledImage.type_id) };
            auto varType{ details::getImageType(type) };

            if (type.array.empty()) {
                textureInfos[name] = { binding , varType };
            }
            else {
                // handle array / array of array
                // https://stackoverflow.com/questions/62031259/specifying-binding-for-texture-arrays-in-glsl
                // TODO: The spriv_cross failed to reflect top level array
                // https://github.com/KhronosGroup/SPIRV-Cross/issues/2421
                std::vector<uint32_t> indices(type.array.size(), 0);
                do {
                    auto nameIndexed = name + details::getArrayIndexStr(indices);
                    textureInfos[nameIndexed] = { binding++, varType };
                } while (details::getNextIndices(type.array, indices));
            }
        }

        return textureInfos;
    }

    static GLProgram::UniformBlockInfoMap reflectUniformBlockInfos(
        spirv_cross::Compiler const& compiler,
        spirv_cross::SmallVector<spirv_cross::Resource> const& uniformBuffers) {
        GLProgram::UniformBlockInfoMap uniformBufferInfos;

        for (auto const& uniformBuffer : uniformBuffers) {
            // name
            std::string name{ uniformBuffer.name };
            if (name.empty()) {
                std::cerr << "Shader uniform block doesn't have name, fallback..." << std::endl;
                name = compiler.get_fallback_name(spirv_cross::ID(uniformBuffer.base_type_id));
            }

            // binding
            int binding{ 0 };
            spirv_cross::Bitset mask{ compiler.get_decoration_bitset(uniformBuffer.id) };
            if (mask.get(spv::DecorationBinding)) {
                compiler.get_decoration(uniformBuffer.id, spv::DecorationBinding);
            }
            else {
                std::cerr << "Cannot get binding for storage buffer " << name << std::endl;
            }

            // size
            auto const& type{ compiler.get_type(uniformBuffer.base_type_id) };
            uint32_t size{ static_cast<uint32_t>(compiler.get_declared_struct_size(type)) };

            uniformBufferInfos[name] = { binding, size };
        }

        return uniformBufferInfos;
    }

    static GLProgram::StorageBufferInfoMap reflectStorageBufferInfos(
        spirv_cross::Compiler const& compiler,
        spirv_cross::SmallVector<spirv_cross::Resource> const& storageBuffers) {
        GLProgram::StorageBufferInfoMap storageBufferInfos;

        for (auto const& storageBuffer : storageBuffers) {
            // name
            std::string name{ storageBuffer.name };
            if (name.empty()) {
                std::cerr << "Storage block doesn't have name, fallback..." << std::endl;
                name = compiler.get_fallback_name(spirv_cross::ID(storageBuffer.base_type_id));
            }

            // binding
            int binding{ 0 };
            spirv_cross::Bitset mask{ compiler.get_decoration_bitset(storageBuffer.id) };
            if (mask.get(spv::DecorationBinding)) {
                compiler.get_decoration(storageBuffer.id, spv::DecorationBinding);
            }
            else {
                std::cerr << "Cannot get binding for storage buffer " << name << std::endl;
            }

            storageBufferInfos[name] = { binding };
        }

        return storageBufferInfos;
    }

    static GLProgram::StorageImageInfoMap reflectImageInfos(
        spirv_cross::Compiler const& compiler,
        spirv_cross::SmallVector<spirv_cross::Resource> const& storageImages) {
        GLProgram::StorageImageInfoMap storageImageInfos;

        for (auto const& storageImage : storageImages) {
            // name
            std::string name{ storageImage.name };
            if (name.empty()) {
                std::cerr << "Storage image doesn't have name, fallback..." << std::endl;
                name = compiler.get_fallback_name(spirv_cross::ID(storageImage.id));
            }

            // binding
            int binding{ 0 };
            spirv_cross::Bitset mask{ compiler.get_decoration_bitset(storageImage.id) };
            if (mask.get(spv::DecorationBinding)) {
                compiler.get_decoration(storageImage.id, spv::DecorationBinding);
            }
            else {
                std::cerr << "Cannot get binding for storage image " << name << std::endl;
            }

            // type
            auto type{ compiler.get_type(storageImage.base_type_id) };
            storageImageInfos[name] = { binding, getImageType(type) };
        }

        return storageImageInfos;
    }

    static GLProgram::AtomicCounterInfoMap reflectAtomicCounterBufferInfos(
        spirv_cross::Compiler const& compiler,
        spirv_cross::SmallVector<spirv_cross::Resource> const& atomicCounters) {
        GLProgram::AtomicCounterInfoMap atomicCounterInfos;
        for (auto const& atomicCounter : atomicCounters) {
            auto const& type{ compiler.get_type(atomicCounter.type_id) };

            // name
            std::string name{ atomicCounter.name };
            if (name.empty()) {
                std::cerr << "Storage image doesn't have name, fallback..." << std::endl;
                name = compiler.get_fallback_name(spirv_cross::ID(atomicCounter.id));
            }

            // binding
            int binding{ 0 };
            spirv_cross::Bitset mask{ compiler.get_decoration_bitset(atomicCounter.id) };
            if (mask.get(spv::DecorationBinding)) {
                binding = compiler.get_decoration(atomicCounter.id, spv::DecorationBinding);
            }
            else {
                std::cerr << "Cannot get binding for atomic counter " << name << std::endl;
            }

            int offset{ 0 };
            if (mask.get(spv::DecorationOffset)) {
                offset = compiler.get_decoration(atomicCounter.id, spv::DecorationOffset);
            }
            else {
                std::cerr << "Cannot get offset for atomic counter " << name << std::endl;
            }

            atomicCounterInfos[name] = { binding, offset };
        }

        return atomicCounterInfos;
    }
}

void ProgramManager::addIncludeDirectory(std::filesystem::path const& includeDir) {
    if (m_includeDirectories.end() ==
        std::find(m_includeDirectories.begin(), m_includeDirectories.end(), includeDir)) {
        m_includeDirectories.push_back(includeDir);
    }
}

std::shared_ptr<GLProgram> ProgramManager::create(std::vector<ShaderSource> const& shaderSources) {
    using SprivCode = std::vector<uint32_t>;
    using Stage = ShaderModule::Stage;

    shaderc::Compiler compiler;

    std::vector<ShaderModule> shaderModules;
    std::vector<SprivCode> spirvCodes;
    std::map<Stage, GLProgram::UniformVarInfoMap> uniformVarStageInfos;
    std::map<Stage, GLProgram::TextureInfoMap> textureStageInfos;
    std::map<Stage, GLProgram::UniformBlockInfoMap> uniformBlockStageInfos;
    std::map<Stage, GLProgram::StorageBufferInfoMap> storageBufferStageInfos;
    std::map<Stage, GLProgram::StorageImageInfoMap> storageImageStageInfos;
    std::map<Stage, GLProgram::AtomicCounterInfoMap> atomicCounterStageInfos;

    for (auto& shaderSource : shaderSources) {
        shaderc::CompileOptions options;
        options.SetTargetEnvironment(shaderc_target_env_opengl, shaderc_env_version_opengl_4_5);
        options.SetSourceLanguage(shaderc_source_language_glsl);
        //options.SetOptimizationLevel(shaderc_optimization_level_performance);
        options.SetOptimizationLevel(shaderc_optimization_level_zero);
        options.SetIncluder(std::make_unique<details::ShaderIncluder>(*this));

        for (auto const& macro : shaderSource.macros) {
            options.AddMacroDefinition(macro.name, macro.value);
        }

        shaderc_shader_kind kind{ shaderc_glsl_infer_from_source };
        switch (shaderSource.stage) {
        case ShaderModule::Stage::Vertex: kind = shaderc_vertex_shader; break;
        case ShaderModule::Stage::TessControl: kind = shaderc_tess_control_shader; break;
        case ShaderModule::Stage::TessEvaluation: kind = shaderc_tess_evaluation_shader; break;
        case ShaderModule::Stage::Geometry: kind = shaderc_geometry_shader; break;
        case ShaderModule::Stage::Fragment: kind = shaderc_fragment_shader; break;
        case ShaderModule::Stage::Compute: kind = shaderc_compute_shader; break;
        case ShaderModule::Stage::Task: kind = shaderc_task_shader; break;
        case ShaderModule::Stage::Mesh: kind = shaderc_mesh_shader; break;
        case ShaderModule::Stage::RayGen: kind = shaderc_raygen_shader; break;
        case ShaderModule::Stage::RayIntersection: kind = shaderc_intersection_shader; break;
        case ShaderModule::Stage::RayAnyHit: kind = shaderc_anyhit_shader; break;
        case ShaderModule::Stage::RayClosestHit: kind = shaderc_closesthit_shader; break;
        case ShaderModule::Stage::RayMiss: kind = shaderc_miss_shader; break;
        case ShaderModule::Stage::RayCallable: kind = shaderc_callable_shader; break;
        default: break;
        }

        // preprocess the shader file: add proper definitions
        // version string is also processed in this step, but we don't expose the API
        std::string code;
        if (!readFile(shaderSource.filepath, code)) {
            throw std::runtime_error("Cannot ...");
        }

        std::u8string inputFileName{ shaderSource.filepath.u8string().c_str() };

        auto preprocessResult{ compiler.PreprocessGlsl(code, kind, (char*)inputFileName.c_str(), options) };
        if (preprocessResult.GetCompilationStatus() != shaderc_compilation_status_success) {
            std::cerr << "shader preprocess error" << std::endl;
            std::cerr << preprocessResult.GetErrorMessage() << std::endl;
            throw std::runtime_error("Cannot ...");
        }

        code = std::string(preprocessResult.cbegin(), preprocessResult.cend());
        //std::cout << code << std::endl;

        shaderc::SpvCompilationResult compileResult;

        // 1. compile the shader module with zero optimization for reflection
        compileResult = compiler.CompileGlslToSpv(code, kind, (char*)inputFileName.c_str(), options);
        if (compileResult.GetCompilationStatus() != shaderc_compilation_status_success) {
            std::cerr << "shader compile error" << std::endl;
            std::cerr << compileResult.GetErrorMessage() << std::endl;
            throw std::runtime_error("Cannot ...");
        }

        // reflection
        SprivCode spirvReflect{ compileResult.begin(), compileResult.end() };
        //printSpirvReflection(spirvReflect);

        spirv_cross::Compiler compilerReflect{ spirvReflect };
        spirv_cross::ShaderResources resources{ compilerReflect.get_shader_resources() };

        uniformVarStageInfos.emplace(shaderSource.stage,
            details::reflectUniformVarInfos(compilerReflect, resources.gl_plain_uniforms));
        textureStageInfos.emplace(shaderSource.stage,
            details::reflectTextureInfos(compilerReflect, resources.sampled_images));
        uniformBlockStageInfos.emplace(shaderSource.stage,
            details::reflectUniformBlockInfos(compilerReflect, resources.uniform_buffers));
        storageBufferStageInfos.emplace(shaderSource.stage,
            details::reflectStorageBufferInfos(compilerReflect, resources.storage_buffers));
        storageImageStageInfos.emplace(shaderSource.stage,
            details::reflectImageInfos(compilerReflect, resources.storage_images));
        atomicCounterStageInfos.emplace(shaderSource.stage,
            details::reflectAtomicCounterBufferInfos(compilerReflect, resources.atomic_counters));

        // 2. compile the shader module with performance option for runtime 
        options.SetOptimizationLevel(shaderc_optimization_level_performance);
        compileResult = compiler.CompileGlslToSpv(code, kind, (char*)inputFileName.c_str(), options);
        if (compileResult.GetCompilationStatus() != shaderc_compilation_status_success) {
            std::cerr << "shader compile error" << std::endl;
            std::cerr << compileResult.GetErrorMessage() << std::endl;
            throw std::runtime_error("Cannot ...");
        }

        SprivCode spirvRuntime{ compileResult.cbegin(), compileResult.cend() };
        shaderModules.emplace_back(spirvRuntime, shaderSource.stage, shaderSource.entrypoint.c_str());
        spirvCodes.push_back(std::move(spirvRuntime));
    }

    std::shared_ptr<GLProgram> program{ new GLProgram };
    for (auto const& shaderModule : shaderModules) {
        program->attach(shaderModule);
    }

    program->link();

    for (auto const& shaderModule : shaderModules) {
        program->detach(shaderModule);
    }

    m_programInfos.push_back({ program, shaderSources });

    // assemble reflection data from different stage
    // note: in OpenGL, we don't care about different stage, so combine them all together
    for (auto const& [stage, varInfos] : uniformVarStageInfos) {
        for (auto const& [name, varInfo] : varInfos) {
            if (program->m_uniformVarInfos.find(name) == program->m_uniformVarInfos.end()) {
                program->m_uniformVarInfos[name] = varInfo;
            }
        }
    }

    for (auto const& [stage, textureInfos] : textureStageInfos) {
        for (auto const& [name, textureInfo] : textureInfos) {
            if (program->m_textureInfos.find(name) == program->m_textureInfos.end()) {
                program->m_textureInfos[name] = textureInfo;
            }
        }
    }

    for (auto const& [stage, blockInfos] : uniformBlockStageInfos) {
        for (auto const& [name, blockInfo] : blockInfos) {
            if (program->m_uniformBlockInfos.find(name) == program->m_uniformBlockInfos.end()) {
                program->m_uniformBlockInfos[name] = blockInfo;
            }
        }
    }

    for (auto const& [stage, bufferInfos] : storageBufferStageInfos) {
        for (auto const& [name, bufferInfo] : bufferInfos) {
            if (program->m_storageBufferInfos.find(name) == program->m_storageBufferInfos.end()) {
                program->m_storageBufferInfos[name] = bufferInfo;
            }
        }
    }

    for (auto const& [stage, imageInfos] : storageImageStageInfos) {
        for (auto const& [name, imageInfo] : imageInfos) {
            if (program->m_storageImageInfos.find(name) == program->m_storageImageInfos.end()) {
                program->m_storageImageInfos[name] = imageInfo;
            }
        }
    }

    for (auto const& [stage, atomicInfos] : atomicCounterStageInfos) {
        for (auto const& [name, atomicInfo] : atomicInfos) {
            if (program->m_atomicCounterInfos.find(name) == program->m_atomicCounterInfos.end()) {
                program->m_atomicCounterInfos[name] = atomicInfo;
            }
        }
    }

    return program;
}

void ProgramManager::remove(GLProgram& program) {

}

void ProgramManager::reload(GLProgram& program) {

}

void ProgramManager::printSpirvReflection(std::vector<uint32_t> const& spirv) {
    spirv_cross::CompilerGLSL::Options options;
    options.version = 450;
    options.es = false;
    options.enable_420pack_extension = false;

    spirv_cross::CompilerGLSL compiler(spirv);
    compiler.set_common_options(options);

    auto printResources = [](spirv_cross::CompilerGLSL& compiler, const std::string& tag,
        const spirv_cross::SmallVector<spirv_cross::Resource>& resources) {
            // @ref spirv-cross/main.cpp print_resources
            printf("%s\n", tag.c_str());
            bool isSSBO{ tag == "ssbos" };
            for (auto& res : resources) {
                auto& type = compiler.get_type(res.type_id);

                if (isSSBO && compiler.buffer_is_hlsl_counter_buffer(res.id)) {
                    continue;
                }

                // If we don't have a name, use the fallback for the type instead of the variable
                // for SSBOs and UBOs since those are the only meaningful names to use externally.
                bool isPushConstant{
                    compiler.get_storage_class(res.id) == spv::StorageClass::StorageClassPushConstant };

                bool isBlock{ compiler.get_decoration_bitset(type.self).get(spv::DecorationBlock) ||
                    compiler.get_decoration_bitset(type.self).get(spv::DecorationBufferBlock) };

                bool isSizedBlock{ isBlock &&
                    (compiler.get_storage_class(res.id) == spv::StorageClass::StorageClassUniform ||
                     compiler.get_storage_class(res.id) == spv::StorageClass::StorageClassUniformConstant) };

                //std::cout << "isPushConstant: " << isPushConstant << std::endl;
                //std::cout << "isBlock: " << isBlock << std::endl;
                //std::cout << "isSizedBlock: " << isSizedBlock << std::endl;

                spirv_cross::ID fallback_id{ !isPushConstant && isBlock ?
                    spirv_cross::ID(res.base_type_id) : spirv_cross::ID(res.id) };

                uint32_t blockSize{ 0 };
                uint32_t runtimeArrayStride{ 0 };
                if (isSizedBlock) {
                    auto& base_type{ compiler.get_type(res.base_type_id) };
                    blockSize = uint32_t(compiler.get_declared_struct_size(base_type));
                    runtimeArrayStride = uint32_t(
                        compiler.get_declared_struct_size_runtime_array(base_type, 1) -
                        compiler.get_declared_struct_size_runtime_array(base_type, 0));
                }

                spirv_cross::Bitset mask{ isSSBO ?
                    compiler.get_buffer_block_flags(res.id) : compiler.get_decoration_bitset(res.id) };

                std::string array;
                for (auto arr : type.array) {
                    array = spirv_cross::join("[", arr ? spirv_cross::convert_to_string(arr) : "", "]") + array;
                }

                printf(" ID %03u : %s%s",
                    uint32_t(res.id),
                    !res.name.empty() ?
                    res.name.c_str() :
                    compiler.get_fallback_name(fallback_id).c_str(), array.c_str());

                if (mask.get(spv::DecorationLocation)) {
                    printf(" (Location : %u)", compiler.get_decoration(res.id, spv::DecorationLocation));
                }

                if (mask.get(spv::DecorationDescriptorSet)) {
                    printf(" (Set : %u)", compiler.get_decoration(res.id, spv::DecorationDescriptorSet));
                }

                if (mask.get(spv::DecorationBinding)) {
                    printf(" (Binding : %u)", compiler.get_decoration(res.id, spv::DecorationBinding));
                }

                if (static_cast<const spirv_cross::CompilerGLSL&>(compiler).variable_is_depth_or_compare(res.id)) {
                    printf(" (comparison)");
                }

                if (mask.get(spv::DecorationInputAttachmentIndex)) {
                    printf(" (Attachment : %u)",
                        compiler.get_decoration(res.id, spv::DecorationInputAttachmentIndex));
                }

                if (mask.get(spv::DecorationNonReadable)) {
                    printf(" writeonly");
                }

                if (mask.get(spv::DecorationNonWritable)) {
                    printf(" readonly");
                }

                if (mask.get(spv::DecorationRestrict)) {
                    printf(" restrict");
                }

                if (mask.get(spv::DecorationCoherent)) {
                    printf(" coherent");
                }

                if (mask.get(spv::DecorationVolatile)) {
                    printf(" volatile");
                }

                if (isSizedBlock) {
                    printf(" (BlockSize : %u bytes)", blockSize);
                    if (runtimeArrayStride) {
                        printf(" (Unsized array stride: %u bytes)", runtimeArrayStride);
                    }
                }

                uint32_t counter_id = 0;
                if (isSSBO && compiler.buffer_get_hlsl_counter_buffer(res.id, counter_id)) {
                    printf(" (HLSL counter buffer ID: %u)", counter_id);
                }

                printf("\n");
                for (size_t i = 0; i < type.member_types.size(); ++i) {
                    const auto& memberType{ compiler.get_type(type.member_types[i]) };
                    std::string memberName{ compiler.get_member_name(type.self, i) };
                    if (memberType.basetype != spirv_cross::SPIRType::Struct) {
                        printf("    %s%dx%d %s",
                            details::getTypeStr(memberType).c_str(),
                            memberType.vecsize, memberType.columns,
                            memberName.c_str());
                        if (!memberType.array.empty()) {
                            for (auto d : memberType.array) {
                                printf("[%d]", d);
                            }
                        }

                        printf("\n");
                    }

                    std::cout << "      memberType.vecsize: " << memberType.vecsize << std::endl;
                    std::cout << "      memberType.columns: " << memberType.columns << std::endl;
                    std::cout << "      memberType.array.size(): " << memberType.array.size() << std::endl;
                }
            }
            printf("\n");
        };

    spirv_cross::ShaderResources resources = compiler.get_shader_resources();
    // OpenGL don't have subpass inputs
    //printResources(compiler, "subpass inputs", resources.subpass_inputs);
    // We don't care about the input / output
    //printResources(compiler, "inputs", resources.stage_inputs);
    //printResources(compiler, "outputs", resources.stage_outputs);
    printResources(compiler, "textures", resources.sampled_images);
    // OpenGL don't have seperate images and samplers
    //printResources(compiler, "separate images", resources.separate_images);
    //printResources(compiler, "separate samplers", resources.separate_samplers);
    printResources(compiler, "images", resources.storage_images);
    printResources(compiler, "ssbos", resources.storage_buffers);
    printResources(compiler, "ubos", resources.uniform_buffers);
    // OpenGL don't have push constant
    //printResources(compiler, "push", resources.push_constant_buffers);
    printResources(compiler, "counters", resources.atomic_counters);
    // OpenGL don't have acceleration structures(Ray Tracing)
    //printResources(compiler, "acceleration structures", resources.acceleration_structures);
    // OpenGL don't have shader record buffers(Ray Tracing)
    //printResources(compiler, "record buffers", resources.shader_record_buffers);
    printResources(compiler, "uniforms", resources.gl_plain_uniforms);
}

bool ProgramManager::readFile(std::filesystem::path const& filepath, std::string& content) noexcept {
    try {
        std::ifstream fp;
        fp.exceptions(std::ifstream::failbit | std::ifstream::badbit);

        fp.open(filepath, std::ios::in | std::ios::binary);

        fp.seekg(0, std::ios::end);
        std::streampos size{ fp.tellg() };

        content.resize(size);
        fp.seekg(0, std::ios::beg);
        fp.read(content.data(), size);
    }
    catch (std::ifstream::failure& e) {
        std::cerr << "read " << filepath << " error: " << e.what() << std::endl;
        return false;
    }
    catch (...) {
        std::cerr << "read " << filepath << " error" << std::endl;
        return false;
    }

    return true;
}