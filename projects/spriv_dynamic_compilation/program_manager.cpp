#include "program_manager.h"

#include <shaderc/shaderc.hpp>
#include <spirv_cross.hpp>
#include <spirv_glsl.hpp>

#include <fstream>

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
        case spirv_cross::SPIRType::Unknown : return "????";
        case spirv_cross::SPIRType::Void : return "void";
        case spirv_cross::SPIRType::Boolean : return "bool";
        case spirv_cross::SPIRType::SByte: return "int8";
        case spirv_cross::SPIRType::UByte: return "uint8";
        case spirv_cross::SPIRType::Short: return "int16";
        case spirv_cross::SPIRType::UShort: return "uint16";
        case spirv_cross::SPIRType::Int : return "int";
        case spirv_cross::SPIRType::UInt : return "uint";
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
}

void ProgramManager::addIncludeDirectory(std::filesystem::path const& includeDir) {
    if (m_includeDirectories.end() ==
        std::find(m_includeDirectories.begin(), m_includeDirectories.end(), includeDir)) {
        m_includeDirectories.push_back(includeDir);
    }
}

std::shared_ptr<GLProgram> ProgramManager::create(std::vector<ShaderSource> const& shaderSources) {
    using  SprivCode = std::vector<uint32_t>;

    shaderc::Compiler compiler;

    std::vector<ShaderModule> shaderModules;
    std::vector<SprivCode> spirvCodes;
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

        std::string inputFileName{ shaderSource.filepath.u8string().c_str() };

        auto preprocessResult{ compiler.PreprocessGlsl(code, kind, inputFileName.c_str(), options) };
        if (preprocessResult.GetCompilationStatus() != shaderc_compilation_status_success) {
            std::cerr << "shader preprocess error" << std::endl;
            std::cerr << preprocessResult.GetErrorMessage() << std::endl;
            //return nullptr;
            throw std::runtime_error("Cannot ...");
        }

        code = std::string(preprocessResult.cbegin(), preprocessResult.cend());
        std::cout << code << std::endl;

        auto compileResult{ compiler.CompileGlslToSpv(code, kind, inputFileName.c_str(), options) };
        if (compileResult.GetCompilationStatus() != shaderc_compilation_status_success) {
            std::cerr << "shader compile error" << std::endl;
            std::cerr << compileResult.GetErrorMessage() << std::endl;
            //return nullptr;
            throw std::runtime_error("Cannot ...");
        }

        std::vector<uint32_t> spirv(compileResult.cbegin(), compileResult.cend());
        shaderModules.emplace_back(spirv, shaderSource.stage, shaderSource.entrypoint.c_str());
        spirvCodes.push_back(std::move(spirv));
    }

    // Reflect
    for (size_t i = 0; i < spirvCodes.size(); ++i) {
        std::cout << shaderSources[i].filepath << std::endl;
        printSpirvReflection(spirvCodes[i]);
        //auto outpath{ 
        //    shaderSources[i].filepath.parent_path() / (shaderSources[i].filepath.filename().string() + ".spv") };
        //std::ofstream fp(outpath);
        //fp.write(reinterpret_cast<char const*>(spirvCodes[i].data()), spirvCodes[i].size() * sizeof(uint32_t));
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

                spirv_cross::ID fallback_id{ !isPushConstant && isBlock ?
                    spirv_cross::ID(res.base_type_id) : spirv_cross::ID(res.id) };

                uint32_t blockSize = 0;
                uint32_t runtimeArrayStride = 0;
                if (isSizedBlock) {
                    auto& base_type = compiler.get_type(res.base_type_id);
                    blockSize = uint32_t(compiler.get_declared_struct_size(base_type));
                    runtimeArrayStride = uint32_t(
                        compiler.get_declared_struct_size_runtime_array(base_type, 1) -
                        compiler.get_declared_struct_size_runtime_array(base_type, 0));
                }

                spirv_cross::Bitset mask{ isSSBO ?
                    compiler.get_buffer_block_flags(res.id) : compiler.get_decoration_bitset(res.id) };

                std::string array;
                for (auto arr : type.array)
                    array = spirv_cross::join("[", arr ? spirv_cross::convert_to_string(arr) : "", "]") + array;

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

                        continue;
                    }

                    std::cout << "      memberType.vecsize: " << memberType.vecsize << std::endl;
                    std::cout << "      memberType.columns: " << memberType.columns << std::endl;
                    std::cout << "memberType.array.size(): " << memberType.array.size() << std::endl;
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