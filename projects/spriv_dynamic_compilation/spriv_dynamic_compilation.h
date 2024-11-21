#pragma once

#include <memory>
#include <vector>

#include "../base/application.h"
#include "../base/camera.h"
#include "../base/glsl_program.h"
#include "../base/light.h"
#include "../base/model.h"
#include "../base/uniform_buffer.h"
#include "../base/texture2d.h"

#include "program_manager.h"
#include "material.h"

class SprivDynamicCompilation : public Application {
public:
    SprivDynamicCompilation(const Options& options);

private:
    std::unique_ptr<DirectionalLight> _dirLight;

    std::unique_ptr<Camera> _camera;
    std::unique_ptr<UniformBuffer> _uboCamera;

    std::unique_ptr<Model> _model;

    std::unique_ptr<ProgramManager> _programManager;

    std::shared_ptr<GLProgram> _lambertProgram;

    std::unique_ptr<Material> _lambertMaterial;

    float _array[2]{1.0f, 1.0f};

private:
    void handleInput() override;

    void renderFrame() override;

    void renderUI();

    void initMaterial();
};
