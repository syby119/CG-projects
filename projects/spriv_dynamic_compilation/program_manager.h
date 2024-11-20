#pragma once

#include <filesystem>
#include <memory>
#include <string>
#include <vector>

#include "glad/gl.h"

#include "shader_module.h"
#include "gl_program.h"

class ProgramManager {
public:
    struct MarcoDefinition {
        std::string name;
        std::string value;
    };

    struct ShaderSource {
        ShaderSource(ShaderModule::Stage stage, std::string filepath)
            : stage{ stage }, filepath{ filepath } { }

        ShaderSource(ShaderModule::Stage stage, std::string filepath,
            std::vector<MarcoDefinition> const& macros)
            : stage{ stage }, filepath{ filepath }, macros{ macros } { }

        ShaderSource(ShaderModule::Stage stage, std::string filepath,
            std::string const& entrypoint, std::vector<MarcoDefinition> const& macros)
            : stage{ stage }, filepath{ filepath }, entrypoint{ entrypoint }, macros{ macros } { }

        ShaderModule::Stage stage;
        std::filesystem::path filepath;
        std::string entrypoint{ "main" };
        std::vector<MarcoDefinition> macros;
    };

    struct ProgramInfo {
        std::shared_ptr<GLProgram> program;
        std::vector<ShaderSource> shaderSources;
    };

public:
    ProgramManager() = default;

    void addIncludeDirectory(std::filesystem::path const& includeDir);

    std::vector<std::filesystem::path> const& getIncludeDirectories() const noexcept {
        return m_includeDirectories;
    }

    std::shared_ptr<GLProgram> create(std::vector<ShaderSource> const& sources);

    void remove(GLProgram& program);

    void reload(GLProgram& program);

    void reflect(std::vector<uint32_t> const& spirv);

    void printSpirvReflection(std::vector<uint32_t> const& spirv);

    static bool readFile(std::filesystem::path const& filepath, std::string& content) noexcept;

private:
    std::vector<std::filesystem::path> m_includeDirectories;

    std::vector<ProgramInfo> m_programInfos;
};