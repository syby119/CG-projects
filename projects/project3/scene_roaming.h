#pragma once

#include <memory>
#include <vector>

#include "../base/application.h"
#include "../base/camera.h"
#include "../base/glsl_program.h"
#include "../base/model.h"

class SceneRoaming : public Application {
public:
    SceneRoaming(const Options& options);

    ~SceneRoaming() = default;

    void handleInput() override;

    void renderFrame() override;

private:
    std::vector<std::unique_ptr<Camera>> m_cameras;
    int m_activeCameraIndex = 0;

    std::unique_ptr<Model> m_bunny;
    std::unique_ptr<Model> m_ground;

    std::unique_ptr<GLSLProgram> m_shader;

    void initShader();
};