#pragma once

#include <string>
#include <vector>

class ShaderModule {
public:
    enum class Stage {
        // vertex-fragment pipeline
        Vertex,
        TessControl,
        TessEvaluation,
        Geometry,
        Fragment,
        // compute pipeline
        Compute,
        // mesh shading pipeline
        Task,
        Mesh,
        // raytracing pipeline
        RayGen,
        RayIntersection,
        RayAnyHit,
        RayClosestHit,
        RayMiss,
        RayCallable,
    };

public:
    ShaderModule(std::string const& code, Stage stage);

    ShaderModule(std::vector<uint32_t> const& spirv, Stage stage, char const* entrypoint);
    
    ShaderModule(ShaderModule&& rhs) noexcept;

    ~ShaderModule();

    ShaderModule& operator=(ShaderModule&& rhs) noexcept;

    uint32_t getHandle() const noexcept {
        return m_handle;
    }

private:
    uint32_t m_handle;
    Stage m_stage;
};