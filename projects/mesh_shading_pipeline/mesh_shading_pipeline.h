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
    RenderCase m_renderCase{RenderCase::Traditional};

    std::unique_ptr<DirectionalLight> m_dirLight;

    std::unique_ptr<Camera> m_camera;
    float m_cameraMoveSpeed{1.0f};

    LambertMaterial m_material{glm::vec3(0.8f)};

    std::unique_ptr<Model> m_model;

    static constexpr uint32_t m_vertexBinding{0};
    static constexpr uint32_t m_vertexIndicesBinding{1};
    static constexpr uint32_t m_primitiveIndicesBinding{2};
    static constexpr uint32_t m_meshletBinding{3};
    static constexpr uint32_t m_instanceMatricesBinding{4};
    static constexpr uint32_t m_bvBinding{5};
    static constexpr uint32_t m_lodInfoBinding{6};
    static constexpr uint32_t m_statisticsBinding{7};

    std::unique_ptr<MeshletModel> m_meshletModel;
    std::unique_ptr<ShaderStorageBuffer> m_ssboVerticesBuffer;
    std::unique_ptr<ShaderStorageBuffer> m_ssboVertexIndicesBuffer;
    std::unique_ptr<ShaderStorageBuffer> m_ssboPrimitiveIndicesBuffer;
    std::unique_ptr<ShaderStorageBuffer> m_ssboMeshletBuffer;
    std::unique_ptr<ShaderStorageBuffer> m_ssboMeshletBVBuffer;

    std::unique_ptr<MeshletModelLod> m_meshletModelLod;
    std::unique_ptr<ShaderStorageBuffer> m_ssboVerticesLodBuffer;
    std::unique_ptr<ShaderStorageBuffer> m_ssboVertexIndicesLodBuffer;
    std::unique_ptr<ShaderStorageBuffer> m_ssboPrimitiveIndicesLodBuffer;
    std::unique_ptr<ShaderStorageBuffer> m_ssboMeshletLodBuffer;
    std::unique_ptr<ShaderStorageBuffer> m_ssboMeshletLodInfoBuffer;
    std::unique_ptr<ShaderStorageBuffer> m_ssboMeshletLodBVBuffer;
    float m_maxLodDistance{10.0f};

    std::unique_ptr<ShaderStorageBuffer> m_ssboStatistics;

    uint32_t m_instanceSpanXCount{100};
    uint32_t m_instanceSpanZCount{100};
    std::unique_ptr<ShaderStorageBuffer> m_ssboInstanceMatricesBuffer;

    std::unique_ptr<GLSLProgram> m_traditionalProgram;

    std::unique_ptr<GLSLProgram> m_triangleProgram;

    std::unique_ptr<GLSLProgram> m_meshletProgram;

    std::unique_ptr<GLSLProgram> m_meshletBVProgram;

    std::unique_ptr<GLSLProgram> m_meshlet2Program;

    std::unique_ptr<GLSLProgram> m_instanceProgram;

    std::unique_ptr<GLSLProgram> m_cullProgram;

    std::unique_ptr<GLSLProgram> m_lodProgram;

    std::unique_ptr<GLSLProgram> m_fullProgram;

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
