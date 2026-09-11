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
    std::unique_ptr<PerspectiveCamera> m_camera;

    std::vector<std::unique_ptr<Model>> m_bunnies;
    std::unique_ptr<LambertMaterial> m_bunnyMaterial;

    std::unique_ptr<Model> m_ground;
    std::unique_ptr<LambertMaterial> m_groundMaterial;

    std::unique_ptr<GLSLProgram> m_lambertShader;

    std::unique_ptr<AmbientLight> m_ambientLight;
    std::unique_ptr<DirectionalLight> m_directionalLight;
    std::unique_ptr<PointLight> m_pointLight;

    std::unique_ptr<Model> m_arrow;
    std::unique_ptr<Model> m_sphere;
    std::unique_ptr<GLSLProgram> m_lightShader;

    std::unique_ptr<GLSLProgram> m_directionalDepthShader;

    std::unique_ptr<Framebuffer> m_depthFbo;
    std::unique_ptr<Texture2D> m_depthTexture;
    glm::mat4 m_directionalLightSpaceMatrix;

    std::array<std::unique_ptr<Framebuffer>, cascadeLevels> m_depthCascadeFbos;
    std::unique_ptr<Texture2DArray> m_depthTextureArray;
    std::array<glm::mat4, cascadeLevels> m_directionalLightSpaceMatrices;

    std::unique_ptr<GLSLProgram> m_omnidirectionalDepthShader;

    std::array<std::unique_ptr<Framebuffer>, 6> m_depthCubeFbos;
    std::unique_ptr<TextureCubemap> m_depthCubeTexture;
    std::array<glm::mat4, 6> m_pointLightSpaceMatrices;
    float m_pointLightZfar = 100.0f;

    int m_directionalFilterRadius = 0;
    bool m_enableOmnidirectionalPCF = false;
    bool m_enableCascadeShadowMapping = false;

    DebugView m_debugView = DebugView::None;

    std::unique_ptr<FullscreenQuad> m_quad;
    std::unique_ptr<GLSLProgram> m_quadShader;
    std::unique_ptr<GLSLProgram> m_quadCascadeShader;

    std::unique_ptr<Model> m_cube;
    std::unique_ptr<GLSLProgram> m_cubeShader;

    // TODO: Change the value here
    std::array<float, cascadeLevels> m_cascadeBiasModifiers = {1.0f, 1.0f, 1.0f, 1.0f, 1.0f};

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