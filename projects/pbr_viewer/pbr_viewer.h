#pragma once

#include <map>
#include <memory>
#include <vector>

#include "../base/application.h"
#include "../base/camera.h"
#include "../base/fullscreen_quad.h"
#include "../base/glsl_program.h"
#include "../base/light.h"
#include "../base/texture.h"
#include "../base/uniform_buffer.h"

#include "./camera_controller.h"
#include "./model.h"
#include "./render_object.h"
#include "./skybox.h"

class PbrViewer : public Application {
public:
    PbrViewer(const Options& options);

    ~PbrViewer();

private:
    std::unique_ptr<Model> m_model;
    std::unique_ptr<GLSLProgram> m_pbrShader;

    std::unique_ptr<PerspectiveCamera> m_camera;
    std::unique_ptr<CameraController> m_cameraController;

    std::unique_ptr<DirectionalLight> m_directionalLight;

    std::unique_ptr<Skybox> m_skybox;
    std::unique_ptr<GLSLProgram> m_skyboxShader;

    std::unique_ptr<FullscreenQuad> m_quad;
    std::unique_ptr<GLSLProgram> m_quadShader;

    std::unique_ptr<UniformBuffer> m_uboCamera;
    std::unique_ptr<UniformBuffer> m_uboLights;
    std::unique_ptr<UniformBuffer> m_uboEnvironment;

    std::vector<RenderObject> m_opaqueQueue;
    std::vector<RenderObject> m_alphaQueue;
    std::vector<RenderObject> m_transparentQueue;

    enum class DebugInput : int {
        All = 0,
        Albedo,
        Roughness,
        Metallic,
        Normal,
        Occlusion,
        Emissive,
    };
    enum DebugInput m_debugInput = {DebugInput::All};

    enum class SkyboxRenderMode : int {
        Raw = 0,
        Irradiance,
        Prefilter,
        BrdfLut
    };
    enum SkyboxRenderMode m_skyboxRenderMode = {SkyboxRenderMode::Raw};

private:
    void handleInput() override;

    void renderFrame() override;

    void updateUniforms();

    void enqueueRenderables();

    void enqueueRenderable(const Node& node, glm::mat4 parentGlobalMatrix);

    void drawPrimitive(const Primitive& primitive) const;

    void clearScreen();

    void renderOpaqueQueue() const;

    void renderAlphaQueue() const;

    void renderTransparentQueue() const;

    void renderSkybox() const;

    void renderIrradianceMap() const;

    void renderPrefilterMap() const;

    void renderBrdfLutMap() const;

    void renderUI() const;

    void initShaders();

    void setPbrShaderUniforms(const RenderObject& object) const;

    void setupUniformBufferObjects();

    void confirmBindingPoints();

    void printRenderQueue(
        const std::string& name, const std::vector<RenderObject>& renderQueue) const;
};