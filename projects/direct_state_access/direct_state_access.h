#pragma once

#include "../base/application.h"
#include "../base/camera.h"
#include "../base/light.h"

class DirectStateAccess : public Application {
public:
    DirectStateAccess(const Options& options);

    ~DirectStateAccess();

private:
    GLuint m_fbo{0u};
    GLuint m_colorAttachment{0u};
    GLuint m_depthAttachment{0u};

    GLuint m_vao{0u};
    GLuint m_vertexVbo{0u};
    GLuint m_instanceVbo{0u};
    GLuint m_ibo{0u};

    GLuint m_texture{0u};

    GLuint m_program{0u};

private:
    void handleInput() override;

    void renderFrame() override;

    void initFramebuffer();

    void initGeometry();

    void initTexture();

    void initProgram();
};