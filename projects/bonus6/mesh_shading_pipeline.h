#pragma once

#include <memory>
#include <vector>

#include "../base/application.h"
#include "../base/camera.h"
#include "../base/glsl_program.h"
#include "../base/light.h"
#include "../base/model.h"

#include "meshlet_model.h"
#include "meshlet_model_lod.h"
#include "shader_storage_buffer.h"

class MeshShadingPipeline : public Application {
public:
    enum RenderCase {
        Traditional,
        Triangle,
        Meshlet,
        Meshlet2,
        Instance,
        Cull,
        Lod,
        Full
    };

    struct LambertMaterial {
        glm::vec3 kd;
    };

public:
    MeshShadingPipeline(const Options& options);

private:
    RenderCase _renderCase{ RenderCase::Traditional };

    std::unique_ptr<DirectionalLight> _dirLight;

    std::unique_ptr<Camera> _camera;
    float _cameraMoveSpeed{ 1.0f };

    LambertMaterial _material{ glm::vec3(0.8f) };

    std::unique_ptr<Model> _model;

    static constexpr uint32_t _vertexBinding{ 0 };
    static constexpr uint32_t _vertexIndicesBinding{ 1 };
    static constexpr uint32_t _primitiveIndicesBinding{ 2 };
    static constexpr uint32_t _meshletBinding{ 3 };
    static constexpr uint32_t _instanceMatricesBinding{ 4 };
    static constexpr uint32_t _bvBinding{ 5 };
    static constexpr uint32_t _lodInfoBinding{ 6 };
    static constexpr uint32_t _statisticsBinding{ 7 };

    std::unique_ptr<MeshletModel> _meshletModel;
    std::unique_ptr<ShaderStorageBuffer> _ssboVerticesBuffer;
    std::unique_ptr<ShaderStorageBuffer> _ssboVertexIndicesBuffer;
    std::unique_ptr<ShaderStorageBuffer> _ssboPrimitiveIndicesBuffer;
    std::unique_ptr<ShaderStorageBuffer> _ssboMeshletBuffer;
    std::unique_ptr<ShaderStorageBuffer> _ssboMeshletBVBuffer;

    std::unique_ptr<MeshletModelLod> _meshletModelLod;
    std::unique_ptr<ShaderStorageBuffer> _ssboVerticesLodBuffer;
    std::unique_ptr<ShaderStorageBuffer> _ssboVertexIndicesLodBuffer;
    std::unique_ptr<ShaderStorageBuffer> _ssboPrimitiveIndicesLodBuffer;
    std::unique_ptr<ShaderStorageBuffer> _ssboMeshletLodBuffer;
    std::unique_ptr<ShaderStorageBuffer> _ssboMeshletLodInfoBuffer;
    std::unique_ptr<ShaderStorageBuffer> _ssboMeshletLodBVBuffer;
    float _maxLodDistance{ 10.0f };

    std::unique_ptr<ShaderStorageBuffer> _ssboStatistics;

    uint32_t _instanceSpanXCount{ 100 };
    uint32_t _instanceSpanZCount{ 100 };
    std::unique_ptr<ShaderStorageBuffer> _ssboInstanceMatricesBuffer;

    std::unique_ptr<GLSLProgram> _traditionalProgram;

    std::unique_ptr<GLSLProgram> _triangleProgram;

    std::unique_ptr<GLSLProgram> _meshletProgram;

    std::unique_ptr<GLSLProgram> _meshletBVProgram;

    std::unique_ptr<GLSLProgram> _meshlet2Program;

    std::unique_ptr<GLSLProgram> _instanceProgram;

    std::unique_ptr<GLSLProgram> _cullProgram;

    std::unique_ptr<GLSLProgram> _lodProgram;

    std::unique_ptr<GLSLProgram> _fullProgram;

    template <typename T>
    static constexpr T snapUp(T v, T snapValue) noexcept {
        return (v + snapValue - 1) / snapValue;
    }

private:
    void handleInput() override;

    void renderFrame() override;

    void renderUI();

    void initPrograms();

    void loadModel(const std::string& filepath);

    void loadMeshletModel(const std::string& filepath);

    void loadMeshletModelLod(const std::vector<std::string>& filepaths);

    void initInstanceMatrices();

    void initStatistics();

    void updateInstanceMatrices();

    void updateStatictics();

    void renderTraditional();

    void renderTriangle();

    void renderMeshlet(bool shwoBV);

    void renderMeshlet2();

    void renderInstance();

    void renderCull();

    void renderLod();

    void renderFull();
};
