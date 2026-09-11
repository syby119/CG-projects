#pragma once

#include <memory>

#include "../base/application.h"
#include "../base/camera.h"
#include "../base/gl_utility.h"
#include "../base/glsl_program.h"
#include "../base/instanced_model.h"
#include "../base/light.h"
#include "../base/model.h"
#include "../base/texture2d.h"

struct DrawElementsIndirectCommand {
    unsigned int count;
    unsigned int instanceCount;
    unsigned int firstIndex;
    int baseVertex;
    unsigned int baseInstance;
};

enum class Method {
    CPU,
    GPU
};

struct LineMaterial {
    glm::vec3 color;
    float width;
};

struct LambertMaterial {
    glm::vec3 kd;
    std::shared_ptr<Texture2D> mapKd;
};

class FrustumCulling : public Application {
public:
    FrustumCulling(const Options& options);

    ~FrustumCulling();

private:
    std::unique_ptr<Model> m_planet;

    std::unique_ptr<Model> m_asternoid;
    std::vector<glm::mat4> m_modelMatrices;
    int m_amount = 10000;
    int m_drawAsternoidCount = 0;

    std::unique_ptr<InstancedModel> m_instancedAsternoids;

    std::unique_ptr<LineMaterial> m_lineMaterial;
    std::unique_ptr<LambertMaterial> m_planetMaterial;
    std::unique_ptr<LambertMaterial> m_asternoidMaterial;

    std::unique_ptr<GLSLProgram> m_lineShader;
    std::unique_ptr<GLSLProgram> m_lineInstancedShader;
    std::unique_ptr<GLSLProgram> m_lambertShader;
    std::unique_ptr<GLSLProgram> m_lambertInstancedShader;

    std::unique_ptr<PerspectiveCamera> m_camera;
    const float m_cameraMoveSpeed = 10.0f;
    const float m_cameraRotateSpeed = 0.05f;

    std::unique_ptr<DirectionalLight> m_light;

    std::vector<int> m_visibles;

    bool m_showBoundingBox = false;

    enum Method m_method = Method::CPU;

    // GPU frustum resources
    GLenum m_transformFeedback = {};
    GLenum m_transformFeedbackResultBuffer = {};
    std::unique_ptr<GLSLProgram> m_frustumCullingShader;

    // indirect draw resources
    bool m_indirectDrawEnabled = false;
    std::vector<DrawElementsIndirectCommand> m_indirectDrawCmds;
    GLuint m_indirectBuffer = {};

    void initModelMatrices();

    void initShaders();

    void initGPUCullingResources();

    void renderAsternoids();

    void renderAsternoidsIndirect();

    void handleInput() override;

    void renderFrame() override;
};