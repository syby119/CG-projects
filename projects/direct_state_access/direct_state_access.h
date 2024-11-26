#pragma once

#include "../base/application.h"
#include "../base/camera.h"
#include "../base/light.h"

class DirectStateAccess : public Application {
public:
    DirectStateAccess(const Options& options);

    ~DirectStateAccess();

private:
    GLuint _fbo{ 0u };
    GLuint _colorAttachment{ 0u };
    GLuint _depthAttachment{ 0u };

    GLuint _vao{ 0u };
    GLuint _vertexVbo{ 0u };
    GLuint _instanceVbo{ 0u };
    GLuint _ibo{ 0u };

    GLuint _texture{ 0u };

    GLuint _program{ 0u };

private:
    void handleInput() override;

    void renderFrame() override;

    void initFramebuffer();

    void initGeometry();

    void initTexture();

    void initProgram();
};