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
    std::unique_ptr<DirectionalLight> m_dirLight;
    std::unique_ptr<UniformBuffer> m_uboLights;

    std::unique_ptr<Camera> m_camera;
    std::unique_ptr<UniformBuffer> m_uboCamera;

    std::unique_ptr<Model> m_model;
    std::shared_ptr<Texture2D> m_texture;

    std::unique_ptr<ProgramManager> m_programManager;

    std::shared_ptr<GLProgram> m_lambertProgram;

    std::unique_ptr<Material> m_lambertMaterial;

private:
    void handleInput() override;

    void renderFrame() override;

    void renderUI();

    void initMaterial();

    void renderLightUI();

    void renderMaterialUI();
};
