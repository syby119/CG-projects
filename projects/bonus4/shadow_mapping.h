#pragma once

#include <array>
#include <glm/glm.hpp>
#include <memory>
#include <vector>

#include "../base/application.h"
#include "../base/camera.h"
#include "../base/framebuffer.h"
#include "../base/fullscreen_quad.h"
#include "../base/glsl_program.h"
#include "../base/light.h"
#include "../base/model.h"
#include "../base/texture2d.h"
#include "../base/texture_cubemap.h"

struct LambertMaterial {
    glm::vec3 ka = glm::vec3(0.05f);
    glm::vec3 kd = glm::vec3(0.8f);
};

enum class DebugView {
    None,
    DirectionalLightDepthTexture,
    PointLightDepthTexture,
    CascadeDepthTextureLevel0,
    CascadeDepthTextureLevel1,
    CascadeDepthTextureLevel2,
    CascadeDepthTextureLevel3,
    CascadeDepthTextureLevel4,
};

constexpr int cascadeLevels = 5;

class ShadowMapping : public Application {
public:
    ShadowMapping(const Options& options);

    ~ShadowMapping();

private:
    std::unique_ptr<PerspectiveCamera> _camera;

    std::vector<std::unique_ptr<Model>> _bunnies;
    std::unique_ptr<LambertMaterial> _bunnyMaterial;

    std::unique_ptr<Model> _ground;
    std::unique_ptr<LambertMaterial> _groundMaterial;

    std::unique_ptr<GLSLProgram> _lambertShader;

    std::unique_ptr<AmbientLight> _ambientLight;
    std::unique_ptr<DirectionalLight> _directionalLight;
    std::unique_ptr<PointLight> _pointLight;

    std::unique_ptr<Model> _arrow;
    std::unique_ptr<Model> _sphere;
    std::unique_ptr<GLSLProgram> _lightShader;

    std::unique_ptr<GLSLProgram> _directionalDepthShader;

    std::unique_ptr<Framebuffer> _depthFbo;
    std::unique_ptr<Texture2D> _depthTexture;
    glm::mat4 _directionalLightSpaceMatrix;

    std::array<std::unique_ptr<Framebuffer>, cascadeLevels> _depthCascadeFbos;
    std::unique_ptr<Texture2DArray> _depthTextureArray;
    std::array<glm::mat4, cascadeLevels> _directionalLightSpaceMatrices;

    std::unique_ptr<GLSLProgram> _omnidirectionalDepthShader;

    std::array<std::unique_ptr<Framebuffer>, 6> _depthCubeFbos;
    std::unique_ptr<TextureCubemap> _depthCubeTexture;
    std::array<glm::mat4, 6> _pointLightSpaceMatrices;
    float _pointLightZfar = 100.0f;

    int _directionalFilterRadius = 0;
    bool _enableOmnidirectionalPCF = false;
    bool _enableCascadeShadowMapping = false;

    DebugView _debugView = DebugView::None;

    std::unique_ptr<FullscreenQuad> _quad;
    std::unique_ptr<GLSLProgram> _quadShader;
    std::unique_ptr<GLSLProgram> _quadCascadeShader;

    std::unique_ptr<Model> _cube;
    std::unique_ptr<GLSLProgram> _cubeShader;

    // TODO: Change the value here
    std::array<float, cascadeLevels> _cascadeBiasModifiers = {1.0f, 1.0f, 1.0f, 1.0f, 1.0f};

    void handleInput() override;

    void renderFrame() override;

    void initGround();

    void initShaders();

    void initDepthResources();

    void renderShadowMaps();

    void renderDirectionalLightShadowMap();

    void renderPointLightShadowMap();

    void renderDirectionalLightCascadeShadowMap();

    void renderSceneFromLight(const GLSLProgram& shader);

    void renderScene();

    void renderDebugView();

    void renderUI();

    void updateDirectionalLightSpaceMatrix();

    void updateDirectionalLightSpaceMatrices();

    void updatePointLightSpaceMatrices();

    BoundingBox getSceneBoundingBox() const;

    std::vector<float> getCascadeDistances() const;
};