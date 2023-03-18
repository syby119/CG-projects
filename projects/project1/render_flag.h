#pragma once

#include <memory>

#include "../base/application.h"
#include "../base/glsl_program.h"

#include "star.h"

class RenderFlag : public Application {
public:
    RenderFlag(const Options& options);

    ~RenderFlag() = default;

private:
    std::unique_ptr<Star> _stars[5];

    std::unique_ptr<GLSLProgram> _starShader;

    void handleInput() override;

    void renderFrame() override;
};