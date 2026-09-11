#pragma once

#include "../base/application.h"
#include "../base/camera.h"
#include "../base/framebuffer.h"
#include "../base/fullscreen_quad.h"
#include "../base/glsl_program.h"
#include "../base/light.h"
#include "../base/model.h"
#include "../base/skybox.h"
#include "../base/texture2d.h"

enum class RenderMode {
    AlphaTesting,
    AlphaBlending,
    DepthPeeling
};

struct TransparentMaterial {
    glm::vec3 albedo;
    float ka;
    glm::vec3 kd;
    float transparent;
};

class Transparency : public Application {
public:
    Transparency(const Options& options);

    ~Transparency();

private:
    enum RenderMode m_renderMode = RenderMode::AlphaTesting;

    std::unique_ptr<Model> m_knot;

    std::unique_ptr<TransparentMaterial> m_knotMaterial;

    std::unique_ptr<Texture2D> m_transparentTexture;

    std::unique_ptr<DirectionalLight> m_light;
    std::unique_ptr<PerspectiveCamera> m_camera;

    std::unique_ptr<GLSLProgram> m_alphaTestingShader;
    std::unique_ptr<GLSLProgram> m_alphaBlendingShader;

    // depth peeling resources
    std::unique_ptr<FullscreenQuad> m_fullscreenQuad;

    std::unique_ptr<Framebuffer> m_colorBlendFbo;
    std::unique_ptr<Texture2D> m_colorBlendTexture;
    std::unique_ptr<Framebuffer> m_fbos[2];
    std::unique_ptr<Texture2D> m_colorTextures[2];
    std::unique_ptr<Texture2D> m_depthTextures[2];

    std::unique_ptr<GLSLProgram> m_depthPeelingInitShader;
    std::unique_ptr<GLSLProgram> m_depthPeelingShader;
    std::unique_ptr<GLSLProgram> m_depthPeelingBlendShader;
    std::unique_ptr<GLSLProgram> m_depthPeelingFinalShader;

    GLuint m_queryId = 0;

    void initShaders();

    void initDepthPeelingResources();

    void handleInput() override;

    void renderFrame() override;

    void renderWithAlphaTesting();

    void renderWithAlphaBlending();

    void renderWithDepthPeeling();

    void renderUI();
};