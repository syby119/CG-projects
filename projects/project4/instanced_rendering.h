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
    const float _cameraMoveSpeed = 10.0f;
    const float _cameraRotateSpeed = 0.05f;

    std::unique_ptr<PerspectiveCamera> _camera;

    std::unique_ptr<Model> _planet;
    std::unique_ptr<Model> _asternoid;

    std::unique_ptr<GLSLProgram> _planetShader;
    std::unique_ptr<GLSLProgram> _asternoidShader;
    std::unique_ptr<GLSLProgram> _asternoidInstancedShader;

    GLuint _instanceBuffer = {};

    int _amount = 50000;
    std::vector<glm::mat4> _modelMatrices;

    enum RenderMode _renderMode = RenderMode::Ordinary;

    bool _wireframe = false;

    void initShaders();

    void handleInput() override;

    void renderFrame() override;
};