#pragma once

#include <memory>
#include <string>

#include "../base/application.h"
#include "../base/camera.h"
#include "../base/glsl_program.h"
#include "../base/light.h"
#include "../base/model.h"
#include "../base/skybox.h"
#include "../base/texture2d.h"

enum class RenderMode {
    Simple,
    Blend,
    Checker
};

struct SimpleMaterial {
    std::shared_ptr<Texture2D> mapKd;
};

struct BlendMaterial {
    glm::vec3 kds[2];
    std::shared_ptr<Texture2D> mapKds[2];
    float blend;
};

struct CheckerMaterial {
    int repeat;
    glm::vec3 colors[2];
};

class TextureMapping : public Application {
public:
    TextureMapping(const Options& options);

    ~TextureMapping();

private:
    std::unique_ptr<Model> m_sphere;

    std::unique_ptr<SimpleMaterial> m_simpleMaterial;
    std::unique_ptr<BlendMaterial> m_blendMaterial;
    std::unique_ptr<CheckerMaterial> m_checkerMaterial;

    std::unique_ptr<PerspectiveCamera> m_camera;
    std::unique_ptr<DirectionalLight> m_light;

    std::unique_ptr<GLSLProgram> m_simpleShader;
    std::unique_ptr<GLSLProgram> m_blendShader;
    std::unique_ptr<GLSLProgram> m_checkerShader;

    std::unique_ptr<SkyBox> m_skybox;

    enum RenderMode m_renderMode = RenderMode::Simple;

    void initSimpleShader();

    void initBlendShader();

    void initCheckerShader();

    void handleInput() override;

    void renderFrame() override;
};