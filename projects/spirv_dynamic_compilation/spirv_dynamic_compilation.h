#pragma once

#include <memory>

#include "../base/application.h"
#include "../base/camera.h"
#include "../base/light.h"
#include "../base/model.h"
#include "../base/uniform_buffer.h"
#include "../base/texture2d.h"

#include "program_manager.h"
#include "material.h"

class SpirvDynamicCompilation : public Application {
public:
    SpirvDynamicCompilation(const Options& options);

private:
    std::unique_ptr<DirectionalLight> _dirLight;
    std::unique_ptr<UniformBuffer> _uboLights;

    std::unique_ptr<Camera> _camera;
    std::unique_ptr<UniformBuffer> _uboCamera;

    std::unique_ptr<Model> _model;
    std::shared_ptr<Texture2D> _texture;

    std::unique_ptr<ProgramManager> _programManager;

    std::shared_ptr<GLProgram> _lambertProgram;

    std::unique_ptr<Material> _lambertMaterial;

private:
    void handleInput() override;

    void renderFrame() override;

    void renderUI();

    void initMaterial();

    void renderLightUI();

    void renderMaterialUI();
};
