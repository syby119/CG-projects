#pragma once

#include <memory>
#include <vector>

#include "../base/application.h"
#include "../base/camera.h"
#include "../base/framebuffer.h"
#include "../base/fullscreen_quad.h"
#include "../base/glsl_program.h"
#include "../base/model.h"
#include "../base/skybox.h"
#include "../base/texture2d.h"

#include "bvh.h"
#include "primitive.h"

class RayTracing : public Application {
public:
    RayTracing(const Options& options);

    ~RayTracing();

private:
    std::unique_ptr<Model> m_lucy;

    std::vector<Sphere> m_balls;
    std::vector<Material> m_ballMaterials;

    std::unique_ptr<TextureCubemap> m_skybox;

    std::unique_ptr<FullscreenQuad> m_screenQuad;

    std::unique_ptr<Camera> m_camera;

    std::unique_ptr<GLSLProgram> m_raytracingShader;
    std::unique_ptr<GLSLProgram> m_drawScreenShader;

    std::unique_ptr<Framebuffer> m_sampleFramebuffers[2];
    uint32_t m_currentReadBufferID = 0;
    uint32_t m_currentWriteBufferID = 1;
    uint32_t m_sampleCount = 0;

    std::unique_ptr<Texture2D> m_outFrames[2];
    std::unique_ptr<Texture2D> m_rngStates[2];

    std::unique_ptr<Texture2D> m_vertexBuffer;
    std::unique_ptr<Texture2D> m_indexBuffer;

    std::unique_ptr<Texture2D> m_sphereBuffer;
    std::unique_ptr<Texture2D> m_primitiveBuffer;

    std::unique_ptr<Texture2D> m_materialBuffer;
    std::unique_ptr<Texture2D> m_bvhBuffer;

    bool m_hasSphere = false;
    bool m_useBVH = false;

    int m_renderSceneIndex = 0;

    void handleInput() override;

    void renderFrame() override;

    void initShaders();

    void createBalls();

    void createRenderScene(int index);

    void createScene1();

    void createScene2();

    void createScene3();

    void createPrimitiveBuffer(
        const std::vector<Sphere>& spheres, const std::vector<Model*> models,
        const std::vector<glm::mat4>& transforms, const std::vector<Material>& sphereMaterials,
        const std::vector<Material>& triangleMaterials);

    int getBufferHeight(size_t nObjects, size_t objectSize, size_t texComponent) const;

    static int toFloatLayout(int v);

    static size_t roundUp(size_t val, size_t number);
};