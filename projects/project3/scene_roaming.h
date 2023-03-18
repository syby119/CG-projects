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
    std::vector<std::unique_ptr<Camera>> _cameras;
    int activeCameraIndex = 0;

    std::unique_ptr<Model> _bunny;

    std::unique_ptr<GLSLProgram> _shader;

    void initShader();
};