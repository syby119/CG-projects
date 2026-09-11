#pragma once

#include <memory>

#include "../base/application.h"
#include "../base/camera.h"
#include "../base/framebuffer.h"
#include "../base/fullscreen_quad.h"
#include "../base/glsl_program.h"
#include "../base/light.h"
#include "../base/model.h"
#include "../base/texture2d.h"

class PostProcessing : public Application {
public:
    PostProcessing(const Options& options);

    ~PostProcessing();

private:
    std::unique_ptr<Model> m_bunny;
    std::unique_ptr<Model> m_cube;

    std::unique_ptr<GLSLProgram> m_drawScreenShader;
    std::unique_ptr<FullscreenQuad> m_screenQuad;

    std::unique_ptr<Camera> m_camera;

    std::unique_ptr<PointLight> m_pointLight;
    std::unique_ptr<Model> m_sphere;

    // deferred rendering: geometry pass resources
    std::unique_ptr<Framebuffer> m_gBufferFBO;
    std::unique_ptr<GLSLProgram> m_gBufferShader;
    std::unique_ptr<Texture2D> m_gPosition;
    std::unique_ptr<Texture2D> m_gNormal;
    std::unique_ptr<Texture2D> m_gAlbedo;
    std::unique_ptr<Texture2D> m_gDepth;

    // SSAO resources
    std::unique_ptr<Texture2D> m_ssaoNoise;
    std::unique_ptr<Texture2D> m_ssaoResult[2];
    std::unique_ptr<Framebuffer> m_ssaoFBO;
    std::unique_ptr<Framebuffer> m_ssaoBlurFBO;

    std::vector<glm::vec3> m_sampleVecs;

    std::unique_ptr<GLSLProgram> m_ssaoShader;
    std::unique_ptr<GLSLProgram> m_ssaoBlurShader;
    std::unique_ptr<GLSLProgram> m_ssaoLightingShader;

    // bloom resources
    std::unique_ptr<Framebuffer> m_bloomFBO;
    std::unique_ptr<Framebuffer> m_blurFBO;
    std::unique_ptr<Framebuffer> m_brightColorFBO;

    std::unique_ptr<Texture2D> m_bloomMap;
    std::unique_ptr<Texture2D> m_brightColorMap[2];

    std::unique_ptr<GLSLProgram> m_lightShader;
    std::unique_ptr<GLSLProgram> m_brightColorShader;
    std::unique_ptr<GLSLProgram> m_blurShader;
    std::unique_ptr<GLSLProgram> m_blendShader;

    uint32_t m_currentReadBuffer = 0;
    uint32_t m_currentWriteBuffer = 1;

    bool m_enableBloom = false;
    bool m_enableSSAO = false;

    void handleInput() override;

    void renderFrame() override;

    void initGeometryPassResources();

    void initSSAOPassResources();

    void initBloomPassResources();

    void initShaders();

    void renderScene();

    void renderUI();

    void extractBrightColor(const Texture2D& sceneMap);

    void blurBrightColor();

    void combineSceneMapAndBloomBlur(const Texture2D& sceneMap);
};