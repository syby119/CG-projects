#pragma once

#include <memory>

#include "../base/application.h"
#include "../base/glsl_program.h"
#include "../base/model.h"
#include "../base/texture2d.h"
#include "../base/camera.h"
#include "../base/light.h"
#include "../base/framebuffer.h"
#include "../base/fullscreen_quad.h"

class PostProcessing : public Application {
public:
	PostProcessing(const Options& options);

	~PostProcessing();

private: 
    std::unique_ptr<Model> _bunny;
    std::unique_ptr<Model> _cube;

    std::unique_ptr<GLSLProgram> _drawScreenShader;
    std::unique_ptr<FullscreenQuad> _screenQuad;

    std::unique_ptr<Camera> _camera;
    
    std::unique_ptr<PointLight> _pointLight;
    std::unique_ptr<Model> _sphere;
    
    // deferred rendering: geometry pass resources 
    std::unique_ptr<Framebuffer> _gBufferFBO;
    std::unique_ptr<GLSLProgram> _gBufferShader;
    std::unique_ptr<Texture2D> _gPosition;
    std::unique_ptr<Texture2D> _gNormal;
    std::unique_ptr<Texture2D> _gAlbedo;
    std::unique_ptr<Texture2D> _gDepth;

    // SSAO resources
    std::unique_ptr<Texture2D> _ssaoNoise;
    std::unique_ptr<Texture2D> _ssaoResult[2];
    std::unique_ptr<Framebuffer> _ssaoFBO;
    std::unique_ptr<Framebuffer> _ssaoBlurFBO;
    
    std::vector<glm::vec3> _sampleVecs;

    std::unique_ptr<GLSLProgram> _ssaoShader;
    std::unique_ptr<GLSLProgram> _ssaoBlurShader;
    std::unique_ptr<GLSLProgram> _ssaoLightingShader;

    // bloom resources
    std::unique_ptr<Framebuffer> _bloomFBO;
    std::unique_ptr<Framebuffer> _blurFBO;
    std::unique_ptr<Framebuffer> _brightColorFBO;

    std::unique_ptr<Texture2D> _bloomMap;
    std::unique_ptr<Texture2D> _brightColorMap[2];
    
    std::unique_ptr<GLSLProgram> _lightShader;
    std::unique_ptr<GLSLProgram> _brightColorShader;
    std::unique_ptr<GLSLProgram> _blurShader;
    std::unique_ptr<GLSLProgram> _blendShader;

    uint32_t _currentReadBuffer = 0;
    uint32_t _currentWriteBuffer = 1;

    bool _enableBloom = false;
    bool _enableSSAO = false;

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