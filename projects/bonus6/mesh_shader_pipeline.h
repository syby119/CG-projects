#pragma once

#include <memory>
#include <vector>

#include "../base/application.h"
#include "../base/camera.h"
#include "../base/glsl_program.h"
#include "../base/light.h"
#include "../base/model.h"

#include "model.h"

class MeshShaderPipeline : public Application {
public:
    enum RenderCase {
        Traditional,
        Triangle,
        Meshlet,
    };

public:
    MeshShaderPipeline(const Options& options);

private:
    RenderCase _renderCase{ RenderCase::Meshlet };

    std::unique_ptr<DirectionalLight> _dirLight;

    std::unique_ptr<Camera> _camera;

    std::unique_ptr<Model> _model;

    std::unique_ptr<experimental::Model> _models[5];

    std::unique_ptr<GLSLProgram> _traditionalProgram;

    std::unique_ptr<GLSLProgram> _triangleProgram;

    std::unique_ptr<GLSLProgram> _meshletProgram;

    std::unique_ptr<GLSLProgram> _taskProgram;

    std::unique_ptr<GLSLProgram> _instanceProgram;

    std::unique_ptr<GLSLProgram> _cullProgram;

    std::unique_ptr<GLSLProgram> _lodProgram;

    std::unique_ptr<GLSLProgram> _cullLodProgram;

private:
    void handleInput() override;

    void renderFrame() override;

    void renderUI();

    void initPrograms();

    void initComputeDebug();

    void initTraditionalProgram();

    void initTriangleProgram();

    void initMeshletProgram();

    void initTaskProgram();

    void initInstanceProgram();

    void initCullProgram();

    void initLodProgram();

    void initCullLodProgram();

    void renderTraditional();

    void renderTriangle();

    void renderMeshlet();
};
