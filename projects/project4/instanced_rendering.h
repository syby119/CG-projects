#pragma once

#include <memory>

#include "../base/application.h"
#include "../base/camera.h"
#include "../base/glsl_program.h"
#include "../base/model.h"

enum class RenderMode {
    Ordinary,
    Instanced
};

class InstancedRendering : public Application {
public:
    InstancedRendering(const Options& options);

    ~InstancedRendering();

private:
    const float m_cameraMoveSpeed = 10.0f;
    const float m_cameraRotateSpeed = 0.05f;

    std::unique_ptr<PerspectiveCamera> m_camera;

    std::unique_ptr<Model> m_planet;
    std::unique_ptr<Model> m_asternoid;

    std::unique_ptr<GLSLProgram> m_planetShader;
    std::unique_ptr<GLSLProgram> m_asternoidShader;
    std::unique_ptr<GLSLProgram> m_asternoidInstancedShader;

    GLuint m_instanceBuffer = {};

    int m_amount = 50000;
    std::vector<glm::mat4> m_modelMatrices;

    enum RenderMode m_renderMode = RenderMode::Ordinary;

    bool m_wireframe = false;

    void initShaders();

    void handleInput() override;

    void renderFrame() override;
};