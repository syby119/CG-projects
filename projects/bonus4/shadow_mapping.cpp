#include <iostream>
#include <limits>
#include <vector>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include "shadow_mapping.h"

constexpr int shadowMapResolution = 4096;

const std::string bunnyRelPath = "obj/bunny.obj";
const std::string arrowRelPath = "obj/arrow.obj";
const std::string sphereRelPath = "obj/sphere.obj";
const std::string cubeRelPath = "obj/cube.obj";

const std::string lightVsRelPath = "shader/bonus4/light.vert";
const std::string lightFsRelPath = "shader/bonus4/light.frag";

const std::string directionalDepthVsRelPath = "shader/bonus4/directional_depth.vert";
const std::string directionalDepthFsRelPath = "shader/bonus4/directional_depth.frag";

const std::string omnidirectionalDepthVsRelPath = "shader/bonus4/omnidirectional_depth.vert";
const std::string omnidirectionalDepthFsRelPath = "shader/bonus4/omnidirectional_depth.frag";

const std::string lambertVsRelPath = "shader/bonus4/lambert.vert";
const std::string lambertFsRelPath = "shader/bonus4/lambert.frag";

const std::string quadVsRelPath = "shader/bonus4/quad.vert";
const std::string quadFsRelPath = "shader/bonus4/quad.frag";

const std::string cubeVsRelPath = "shader/bonus4/cube.vert";
const std::string cubeFsRelPath = "shader/bonus4/cube.frag";

const std::string quadCsmVsRelPath = "shader/bonus4/quad_csm.vert";
const std::string quadCsmFsRelPath = "shader/bonus4/quad_csm.frag";

ShadowMapping::ShadowMapping(const Options& options) : Application(options) {
    // init bunnies
    for (int i = 0; i < 9; ++i) {
        if (i == 0) {
            m_bunnies.emplace_back(new Model(getAssetFullPath(bunnyRelPath)));
        } else {
            m_bunnies.emplace_back(new Model(m_bunnies[0]->getVertices(), m_bunnies[0]->getIndices()));
        }
        m_bunnies[i]->transform.position.y = 2.5f;
        m_bunnies[i]->transform.position.x = 20.0f * (i % 3 - 1);
        m_bunnies[i]->transform.position.z = 20.0f * (i / 3 - 1);
    }
    m_bunnyMaterial.reset(new LambertMaterial);
    m_bunnyMaterial->kd = glm::vec3(1.0f);

    // init arrow and sphere for light representations
    m_arrow.reset(new Model(getAssetFullPath(arrowRelPath)));
    m_sphere.reset(new Model(getAssetFullPath(sphereRelPath)));

    // init ground
    initGround();

    // init camera
    m_camera.reset(new PerspectiveCamera(
        glm::radians(50.0f), 1.0f * m_windowWidth / m_windowHeight, 0.1f, 500.0f));
    m_camera->transform.position.y = 12.0f;
    m_camera->transform.position.z = 12.0f;
    m_camera->fovy = glm::radians(35.0f);
    m_camera->transform.lookAt(glm::vec3(0.0f, 0.0f, -2.0f));

    // init lights
    m_ambientLight.reset(new AmbientLight);

    m_directionalLight.reset(new DirectionalLight);
    m_directionalLight->transform.position = glm::vec3(-10.0f, 10.0f, 10.0f);
    m_directionalLight->transform.scale = glm::vec3(1.0f, 1.0f, 2.0f);
    m_directionalLight->transform.lookAt(glm::vec3(0.0f));
    m_directionalLight->intensity = 0.5f;

    updateDirectionalLightSpaceMatrix();

    updateDirectionalLightSpaceMatrices();

    m_pointLight.reset(new PointLight);
    m_pointLight->transform.position = glm::vec3(-7.5f, 8.0f, 6.0f);
    m_pointLight->transform.scale = glm::vec3(0.2f, 0.2f, 0.2f);
    m_pointLight->intensity = 1.0f;
    m_pointLight->kl = 0.1f;
    m_pointLight->kq = 0.0f;

    updatePointLightSpaceMatrices();

    // init shaders
    initShaders();

    // init framebuffers and depth textures for shadow mapping
    initDepthResources();

    // init quad and cube for debug
    m_quad.reset(new FullscreenQuad);
    m_cube.reset(new Model(getAssetFullPath(cubeRelPath)));

    // init imGUI
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    (void)io;

    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(m_window, true);
#if defined(__EMSCRIPTEN__)
    ImGui_ImplOpenGL3_Init("#version 100");
#elif defined(USE_GLES)
    ImGui_ImplOpenGL3_Init("#version 150");
#else
    ImGui_ImplOpenGL3_Init();
#endif

    // render a frame ahead of time to fix the bug in Ubuntu
    m_enableCascadeShadowMapping = true;
    renderFrame();
    m_enableCascadeShadowMapping = false;
}

ShadowMapping::~ShadowMapping() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

void ShadowMapping::handleInput() {
    if (m_input.keyboard.keyStates[GLFW_KEY_ESCAPE] != GLFW_RELEASE) {
        glfwSetWindowShouldClose(m_window, true);
        return;
    }

    constexpr float cameraMoveSpeed = 50.0f;
    const float cameraMoveDistance = cameraMoveSpeed * m_deltaTime;
    if (m_input.keyboard.keyStates[GLFW_KEY_W] != GLFW_RELEASE) {
        m_camera->transform.position += cameraMoveDistance * m_camera->transform.getFront();
    }

    if (m_input.keyboard.keyStates[GLFW_KEY_A] != GLFW_RELEASE) {
        m_camera->transform.position -= cameraMoveDistance * m_camera->transform.getRight();
    }

    if (m_input.keyboard.keyStates[GLFW_KEY_S] != GLFW_RELEASE) {
        m_camera->transform.position -= cameraMoveDistance * m_camera->transform.getFront();
    }

    if (m_input.keyboard.keyStates[GLFW_KEY_D] != GLFW_RELEASE) {
        m_camera->transform.position += cameraMoveDistance * m_camera->transform.getRight();
    }

    constexpr float lightMoveSpeed = 5.0f;
    bool lightMoved = false;
    if (m_input.keyboard.keyStates[GLFW_KEY_UP] != GLFW_RELEASE) {
        lightMoved = true;
        m_directionalLight->transform.position +=
            lightMoveSpeed * m_directionalLight->transform.getFront() * m_deltaTime;
        m_directionalLight->transform.lookAt(glm::vec3(0.0f));
    }

    if (m_input.keyboard.keyStates[GLFW_KEY_DOWN] != GLFW_RELEASE) {
        lightMoved = true;
        m_directionalLight->transform.position -=
            lightMoveSpeed * m_directionalLight->transform.getFront() * m_deltaTime;
        m_directionalLight->transform.lookAt(glm::vec3(0.0f));
    }

    if (m_input.keyboard.keyStates[GLFW_KEY_LEFT] != GLFW_RELEASE) {
        lightMoved = true;
        m_directionalLight->transform.position -=
            lightMoveSpeed * m_directionalLight->transform.getRight() * m_deltaTime;
        m_directionalLight->transform.lookAt(glm::vec3(0.0f));
    }

    if (m_input.keyboard.keyStates[GLFW_KEY_RIGHT] != GLFW_RELEASE) {
        lightMoved = true;
        m_directionalLight->transform.position +=
            lightMoveSpeed * m_directionalLight->transform.getRight() * m_deltaTime;
        m_directionalLight->transform.lookAt(glm::vec3(0.0f));
    }

    if (lightMoved) {
        updateDirectionalLightSpaceMatrix();
    }

    updateDirectionalLightSpaceMatrices();

    m_input.forwardState();
}

void ShadowMapping::renderFrame() {
    showFpsInWindowTitle();

    glEnable(GL_DEPTH_TEST);

    renderShadowMaps();

    renderScene();

    renderDebugView();

    renderUI();
}

void ShadowMapping::initGround() {
    constexpr float infinity = 100.0f;
    std::vector<Vertex> vertices(4);
    for (size_t i = 0; i < vertices.size(); ++i) {
        vertices[i].position.x = (i % 2) ? infinity : -infinity;
        vertices[i].position.y = 0.0f;
        vertices[i].position.z = (i > 1) ? infinity : -infinity;
        vertices[i].normal = glm::vec3(0.0f, 1.0f, 0.0f);
        vertices[i].texCoord.x = (i % 2) ? 1.0f : 0.0f;
        vertices[i].texCoord.y = (i > 1) ? 1.0f : 0.0f;
    };

    std::vector<uint32_t> indices = {0, 1, 2, 1, 2, 3};

    m_ground.reset(new Model(vertices, indices));

    m_groundMaterial.reset(new LambertMaterial);
    m_groundMaterial->kd = glm::vec3(0.8f);
}

void ShadowMapping::initShaders() {
    const char* version =
#ifdef USE_GLES
        "300 es"
#else
        "330 core"
#endif
        ;

    // depth shader for directional light
    m_directionalDepthShader.reset(new GLSLProgram);
    m_directionalDepthShader->attachVertexShaderFromFile(
        getAssetFullPath(directionalDepthVsRelPath), version);
    m_directionalDepthShader->attachFragmentShaderFromFile(
        getAssetFullPath(directionalDepthFsRelPath), version);
    m_directionalDepthShader->link();

    // depth shader for point light
    m_omnidirectionalDepthShader.reset(new GLSLProgram);
    m_omnidirectionalDepthShader->attachVertexShaderFromFile(
        getAssetFullPath(omnidirectionalDepthVsRelPath), version);
    m_omnidirectionalDepthShader->attachFragmentShaderFromFile(
        getAssetFullPath(omnidirectionalDepthFsRelPath), version);
    m_omnidirectionalDepthShader->link();

    // lambert shader
    // TODO: change the lambert.frag code to render soft shadows, including
    // + shadow mapping for the directional light
    // + omnidirectional shadow mapping for the point light
    // + cascade shadow mapping for the directional light
    m_lambertShader.reset(new GLSLProgram);
    m_lambertShader->attachVertexShaderFromFile(getAssetFullPath(lambertVsRelPath), version);
    m_lambertShader->attachFragmentShaderFromFile(getAssetFullPath(lambertFsRelPath), version);
    m_lambertShader->link();

    // light shader
    m_lightShader.reset(new GLSLProgram);
    m_lightShader->attachVertexShaderFromFile(getAssetFullPath(lightVsRelPath), version);
    m_lightShader->attachFragmentShaderFromFile(getAssetFullPath(lightFsRelPath), version);
    m_lightShader->link();

    // debugging shaders
    // 1. quad shader for directional light depth visualization
    m_quadShader.reset(new GLSLProgram);
    m_quadShader->attachVertexShaderFromFile(getAssetFullPath(quadVsRelPath), version);
    m_quadShader->attachFragmentShaderFromFile(getAssetFullPath(quadFsRelPath), version);
    m_quadShader->link();

    // 2. cube shader for point light depth visualization
    m_cubeShader.reset(new GLSLProgram);
    m_cubeShader->attachVertexShaderFromFile(getAssetFullPath(cubeVsRelPath), version);
    m_cubeShader->attachFragmentShaderFromFile(getAssetFullPath(cubeFsRelPath), version);
    m_cubeShader->link();

    // 3. quad cascade shader for directional light cascade depth visualization
    m_quadCascadeShader.reset(new GLSLProgram);
    m_quadCascadeShader->attachVertexShaderFromFile(getAssetFullPath(quadCsmVsRelPath), version);
    m_quadCascadeShader->attachFragmentShaderFromFile(getAssetFullPath(quadCsmFsRelPath), version);
    m_quadCascadeShader->link();
}

void ShadowMapping::initDepthResources() {
    // Create depth textures and its corresponding framebuffer.
    // Although one can use one framebuffer to handle everything,
    // I choose to use different framebuffers to avoid switching binding textures,
    // as framebuffer texture binding in WebGL is very verbose without glFramebufferTexture
    GLint internalFormat =
#ifdef USE_GLES
        GL_DEPTH_COMPONENT32F
#else
        GL_DEPTH_COMPONENT
#endif
        ;

#ifdef USE_GLES
    _defaultColorTexture.reset(
        new Texture2D(GL_RGB, shadowMapResolution, shadowMapResolution, GL_RGB, GL_UNSIGNED_BYTE));
#endif

    std::vector<float> borderColor = {1.0f, 1.0f, 1.0f, 1.0f};

    // directional light shadow map
    // init depth texture and its corresponding framebuffer
    m_depthTexture.reset(new Texture2D(
        internalFormat, shadowMapResolution, shadowMapResolution, GL_DEPTH_COMPONENT, GL_FLOAT));
    m_depthTexture->bind();
    m_depthTexture->setParamterInt(GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    m_depthTexture->setParamterInt(GL_TEXTURE_MAG_FILTER, GL_NEAREST);
#ifndef __EMSCRIPTEN__
    m_depthTexture->setParamterInt(GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    m_depthTexture->setParamterInt(GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    m_depthTexture->setParamterFloatVector(GL_TEXTURE_BORDER_COLOR, borderColor);
#endif
    m_depthTexture->unbind();

    m_depthFbo.reset(new Framebuffer);
    m_depthFbo->bind();
    m_depthFbo->drawBuffer(GL_NONE);
    m_depthFbo->readBuffer(GL_NONE);

#ifdef USE_GLES
    m_depthFbo->attachTexture2D(*_defaultColorTexture, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D);
#endif
    m_depthFbo->attachTexture2D(*m_depthTexture, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D);

    GLenum status = m_depthFbo->checkStatus();
    if (status != GL_FRAMEBUFFER_COMPLETE) {
        throw std::runtime_error("depthFbo illegal status: " + m_depthFbo->getDiagnostic(status));
    }

    m_depthFbo->unbind();

    // point light shadow map
    // init depth cube texture and its corresponding framebuffers
    m_depthCubeTexture.reset(new TextureCubemap(
        internalFormat, shadowMapResolution, shadowMapResolution, GL_DEPTH_COMPONENT, GL_FLOAT));
    m_depthCubeTexture->bind();
    m_depthCubeTexture->setParamterInt(GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    m_depthCubeTexture->setParamterInt(GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    m_depthCubeTexture->setParamterInt(GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    m_depthCubeTexture->setParamterInt(GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    m_depthCubeTexture->setParamterInt(GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    m_depthCubeTexture->unbind();

    for (size_t i = 0; i < m_depthCubeFbos.size(); ++i) {
        m_depthCubeFbos[i].reset(new Framebuffer);
        m_depthCubeFbos[i]->bind();
        m_depthCubeFbos[i]->drawBuffer(GL_NONE);
        m_depthCubeFbos[i]->readBuffer(GL_NONE);

#ifdef USE_GLES
        m_depthCubeFbos[i]->attachTexture2D(
            *_defaultColorTexture, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D);
#endif
        m_depthCubeFbos[i]->attachTexture2D(
            *m_depthCubeTexture, GL_DEPTH_ATTACHMENT,
            GL_TEXTURE_CUBE_MAP_POSITIVE_X + static_cast<int>(i));

        GLenum status = m_depthCubeFbos[i]->checkStatus();
        if (status != GL_FRAMEBUFFER_COMPLETE) {
            throw std::runtime_error(
                "m_depthCubeFbos illegal status: " + m_depthCubeFbos[i]->getDiagnostic(status));
        }

        m_depthCubeFbos[i]->unbind();
    }

    // directional light cascade shadow map
    // init depth texture array and its corresponding frramebuffers
    m_depthTextureArray.reset(new Texture2DArray(
        internalFormat, shadowMapResolution, shadowMapResolution,
        static_cast<int>(m_directionalLightSpaceMatrices.size()), GL_DEPTH_COMPONENT, GL_FLOAT));
    m_depthTextureArray->bind();
    m_depthTextureArray->setParamterInt(GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    m_depthTextureArray->setParamterInt(GL_TEXTURE_MAG_FILTER, GL_NEAREST);
#ifndef __EMSCRIPTEN__
    m_depthTextureArray->setParamterInt(GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    m_depthTextureArray->setParamterInt(GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    m_depthTextureArray->setParamterFloatVector(GL_TEXTURE_BORDER_COLOR, borderColor);
#endif
    m_depthTextureArray->unbind();

    for (size_t i = 0; i < m_depthCascadeFbos.size(); ++i) {
        m_depthCascadeFbos[i].reset(new Framebuffer);
        m_depthCascadeFbos[i]->bind();
        m_depthCascadeFbos[i]->drawBuffer(GL_NONE);
        m_depthCascadeFbos[i]->readBuffer(GL_NONE);

#ifdef USE_GLES
        m_depthCascadeFbos[i]->attachTexture2D(
            *_defaultColorTexture, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D);
#endif
        m_depthCascadeFbos[i]->attachTextureLayer(
            *m_depthTextureArray, GL_DEPTH_ATTACHMENT, static_cast<int>(i));

        GLenum status = m_depthCascadeFbos[i]->checkStatus();
        if (status != GL_FRAMEBUFFER_COMPLETE) {
            throw std::runtime_error(
                "m_depthCascadeFbos illegal status: " + m_depthCascadeFbos[i]->getDiagnostic(status));
        }

        m_depthCascadeFbos[i]->unbind();
    }
}

void ShadowMapping::renderShadowMaps() {
    renderPointLightShadowMap();

    if (m_enableCascadeShadowMapping) {
        renderDirectionalLightCascadeShadowMap();
    } else {
        renderDirectionalLightShadowMap();
    }
}

void ShadowMapping::renderDirectionalLightShadowMap() {
    glViewport(0, 0, shadowMapResolution, shadowMapResolution);

    m_directionalDepthShader->use();
    m_directionalDepthShader->setUniformMat4("lightSpaceMatrix", m_directionalLightSpaceMatrix);

    m_depthFbo->bind();
    glClear(GL_DEPTH_BUFFER_BIT);

    renderSceneFromLight(*m_directionalDepthShader);
    m_depthFbo->unbind();

    glViewport(0, 0, m_windowWidth, m_windowHeight);
}

void ShadowMapping::renderPointLightShadowMap() {
    glViewport(0, 0, shadowMapResolution, shadowMapResolution);

    m_omnidirectionalDepthShader->use();
    m_omnidirectionalDepthShader->setUniformFloat("zFar", m_pointLightZfar);
    m_omnidirectionalDepthShader->setUniformVec3("lightPosition", m_pointLight->transform.position);

    // render the scene 6 times for each cubemap face
    // m_pointLightSpaceMatrices must be updated before rendering
    for (size_t i = 0; i < m_depthCubeFbos.size(); ++i) {
        m_depthCubeFbos[i]->bind();
        glClear(GL_DEPTH_BUFFER_BIT);

        m_omnidirectionalDepthShader->setUniformMat4(
            "lightSpaceMatrix", m_pointLightSpaceMatrices[i]);
        renderSceneFromLight(*m_omnidirectionalDepthShader);

        m_depthCubeFbos[i]->unbind();
    }

    glViewport(0, 0, m_windowWidth, m_windowHeight);
}

void ShadowMapping::renderDirectionalLightCascadeShadowMap() {
    glViewport(0, 0, shadowMapResolution, shadowMapResolution);

    m_directionalDepthShader->use();

    // render the scene several times for each cascade level
    // m_directionalLightSpaceMatrices must be updated before rendering
    for (size_t i = 0; i < m_depthCascadeFbos.size(); ++i) {
        m_depthCascadeFbos[i]->bind();
        glClear(GL_DEPTH_BUFFER_BIT);

        m_directionalDepthShader->setUniformMat4(
            "lightSpaceMatrix", m_directionalLightSpaceMatrices[i]);
        renderSceneFromLight(*m_directionalDepthShader);

        m_depthCascadeFbos[i]->unbind();
    }

    glViewport(0, 0, m_windowWidth, m_windowHeight);
}

void ShadowMapping::renderSceneFromLight(const GLSLProgram& shader) {
    // 1. draw bunnies
    for (size_t i = 0; i < m_bunnies.size(); ++i) {
        shader.setUniformMat4("model", m_bunnies[i]->transform.getLocalMatrix());
        m_bunnies[i]->draw();
    }

    // 2. draw ground
    shader.setUniformMat4("model", m_ground->transform.getLocalMatrix());
    m_ground->draw();
}

void ShadowMapping::renderScene() {
    glClearColor(m_clearColor.r, m_clearColor.g, m_clearColor.b, m_clearColor.a);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glViewport(0, 0, m_windowWidth, m_windowHeight);
    glCullFace(GL_BACK);

    m_lambertShader->use();

    // camera info
    m_lambertShader->setUniformMat4("projection", m_camera->getProjectionMatrix());
    m_lambertShader->setUniformMat4("view", m_camera->getViewMatrix());
    m_lambertShader->setUniformVec3("viewPosition", m_camera->transform.position);

    // lights info
    m_lambertShader->setUniformFloat("ambientLight.intensity", m_ambientLight->intensity);
    m_lambertShader->setUniformVec3("ambientLight.color", m_ambientLight->color);

    m_lambertShader->setUniformVec3(
        "directionalLight.direction", m_directionalLight->transform.getFront());
    m_lambertShader->setUniformFloat("directionalLight.intensity", m_directionalLight->intensity);
    m_lambertShader->setUniformVec3("directionalLight.color", m_directionalLight->color);
    m_lambertShader->setUniformMat4("directionalLightSpaceMatrix", m_directionalLightSpaceMatrix);

    m_lambertShader->setUniformVec3("pointLight.position", m_pointLight->transform.position);
    m_lambertShader->setUniformFloat("pointLight.intensity", m_pointLight->intensity);
    m_lambertShader->setUniformVec3("pointLight.color", m_pointLight->color);
    m_lambertShader->setUniformFloat("pointLight.kc", m_pointLight->kc);
    m_lambertShader->setUniformFloat("pointLight.kl", m_pointLight->kl);
    m_lambertShader->setUniformFloat("pointLight.kq", m_pointLight->kq);
    m_lambertShader->setUniformFloat("pointLightZfar", m_pointLightZfar);

    // pcf
    m_lambertShader->setUniformInt("directionalFilterRadius", m_directionalFilterRadius);
    m_lambertShader->setUniformBool("enableOmnidirectionalPCF", m_enableOmnidirectionalPCF);

    // depth textures
    if (!m_enableCascadeShadowMapping) {
        m_lambertShader->setUniformInt("depthTexture", 0);
        m_depthTexture->bind(0);
        m_lambertShader->setUniformInt("cascadeCount", 0);
    } else {
        m_lambertShader->setUniformInt("depthTextureArray", 2);
        m_depthTextureArray->bind(2);
        m_lambertShader->setUniformInt(
            "cascadeCount", static_cast<int>(m_directionalLightSpaceMatrices.size()));

        std::vector<float> distances = getCascadeDistances();
        for (size_t i = 1; i < distances.size(); ++i) {
            m_lambertShader->setUniformFloat(
                "cascadeZfars[" + std::to_string(i - 1) + "]", distances[i]);
        }

        for (size_t i = 0; i < m_directionalLightSpaceMatrices.size(); ++i) {
            m_lambertShader->setUniformMat4(
                "directionalLightSpaceMatrices[" + std::to_string(i) + "]",
                m_directionalLightSpaceMatrices[i]);
        }

        for (size_t i = 0; i < m_cascadeBiasModifiers.size(); ++i) {
            m_lambertShader->setUniformFloat(
                "cascadeBiasModifiers[" + std::to_string(i) + "]", m_cascadeBiasModifiers[i]);
        }
    }

    m_lambertShader->setUniformInt("depthCubeTexture", 1);
    m_depthCubeTexture->bind(1);

    // 1. draw bunnies
    m_lambertShader->setUniformVec3("material.ka", m_bunnyMaterial->ka);
    m_lambertShader->setUniformVec3("material.kd", m_bunnyMaterial->kd);
    for (size_t i = 0; i < m_bunnies.size(); ++i) {
        m_lambertShader->setUniformMat4("model", m_bunnies[i]->transform.getLocalMatrix());
        m_bunnies[i]->draw();
    }

    // 2. draw ground
    m_lambertShader->setUniformMat4("model", m_ground->transform.getLocalMatrix());
    m_lambertShader->setUniformVec3("material.ka", m_groundMaterial->ka);
    m_lambertShader->setUniformVec3("material.kd", m_groundMaterial->kd);

    m_ground->draw();

    // 3. draw lights
    m_lightShader->use();
    m_lightShader->setUniformMat4("projection", m_camera->getProjectionMatrix());
    m_lightShader->setUniformMat4("view", m_camera->getViewMatrix());

    m_lightShader->setUniformMat4("model", m_directionalLight->transform.getLocalMatrix());
    m_arrow->draw();

    m_lightShader->setUniformMat4("model", m_pointLight->transform.getLocalMatrix());
    m_sphere->draw();
}

void ShadowMapping::renderDebugView() {
    switch (m_debugView) {
    case DebugView::DirectionalLightDepthTexture:
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        m_quadShader->use();
        m_quadShader->setUniformInt("depthTexture", 0);
        m_depthTexture->bind(0);

        m_quad->draw();
        break;
    case DebugView::PointLightDepthTexture:
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        m_cubeShader->use();
        m_cubeShader->setUniformInt("depthCubeTexture", 0);
        m_cubeShader->setUniformMat4("projection", m_camera->getProjectionMatrix());
        m_cubeShader->setUniformMat4("view", m_camera->getViewMatrix());
        m_cubeShader->setUniformMat4("model", m_cube->transform.getLocalMatrix());
        m_depthCubeTexture->bind(0);

        m_cube->draw();
        break;
    case DebugView::CascadeDepthTextureLevel0:
    case DebugView::CascadeDepthTextureLevel1:
    case DebugView::CascadeDepthTextureLevel2:
    case DebugView::CascadeDepthTextureLevel3:
    case DebugView::CascadeDepthTextureLevel4:
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        m_quadCascadeShader->use();
        m_quadCascadeShader->setUniformInt("depthTextureArray", 0);
        m_depthTextureArray->bind(0);
        m_quadCascadeShader->setUniformInt(
            "level",
            static_cast<int>(m_debugView) - static_cast<int>(DebugView::CascadeDepthTextureLevel0));

        m_quad->draw();
        break;
    default: break;
    }
}

void ShadowMapping::renderUI() {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    const auto flags = ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings;

    if (!ImGui::Begin("Control Panel", nullptr, flags)) {
        ImGui::End();
    } else {
        ImGui::Text("directional light");
        ImGui::Separator();
        ImGui::SliderFloat("intensity##1", &m_directionalLight->intensity, 0.0f, 1.0f);
        ImGui::SliderInt("pcf radius", &m_directionalFilterRadius, 0, 3);
        ImGui::Checkbox("enable csm", &m_enableCascadeShadowMapping);

        ImGui::Text("point light");
        ImGui::Separator();
        ImGui::SliderFloat("intensity##2", &m_pointLight->intensity, 0.0f, 1.0f);
        ImGui::Checkbox("enable pcf", &m_enableOmnidirectionalPCF);

        ImGui::Text("view shadow map");
        ImGui::Separator();

        static const char* debugViewItems[] = {
            "None",
            "directional",
            "omnidirectional",
            "cascade level 0",
            "cascade level 1",
            "cascade level 2",
            "cascade level 3",
            "cascade level 4",
        };

        ImGui::Combo("##2", (int*)(&m_debugView), debugViewItems, IM_ARRAYSIZE(debugViewItems));

        ImGui::End();
    }

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void ShadowMapping::updateDirectionalLightSpaceMatrix() {
    m_directionalLightSpaceMatrix = glm::ortho(-10.0f, 10.0f, -10.0f, 10.0f, 0.1f, 100.0f)
                                   *            // projection
                                   glm::lookAt( // view
                                       m_directionalLight->transform.position,
                                       glm::vec3(0.0f, 0.0f, 0.0f), Transform::getDefaultUp());
}

void ShadowMapping::updateDirectionalLightSpaceMatrices() {
    const BoundingBox box = getSceneBoundingBox();
    std::vector<float> distances = getCascadeDistances();

    for (size_t i = 1; i < distances.size(); ++i) {
        // TODO: change the code here to get light space matrices for CSM
        // --------------------------------------------------------------
        m_directionalLightSpaceMatrices[i - 1] = glm::mat4(1.0f);
        // --------------------------------------------------------------
    }
}

void ShadowMapping::updatePointLightSpaceMatrices() {
    const glm::mat4 projection = glm::perspective(glm::radians(90.0f), 1.0f, 1.0f, m_pointLightZfar);

    const glm::vec3& eye = m_pointLight->transform.position;
    const glm::mat4 views[6] = {
        glm::lookAt(eye, eye + glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f)),
        glm::lookAt(eye, eye + glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f)),
        glm::lookAt(eye, eye + glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f)),
        glm::lookAt(eye, eye + glm::vec3(0.0f, -1.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f)),
        glm::lookAt(eye, eye + glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, -1.0f, 0.0f)),
        glm::lookAt(eye, eye + glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, -1.0f, 0.0f)),
    };

    for (size_t i = 0; i < m_pointLightSpaceMatrices.size(); ++i) {
        m_pointLightSpaceMatrices[i] = projection * views[i];
    }
}

BoundingBox ShadowMapping::getSceneBoundingBox() const {
    auto getModelBoundingBox = [](const Model& model) {
        BoundingBox result;

        BoundingBox box = model.getBoundingBox();
        const glm::mat3 modelMatrix = model.transform.getLocalMatrix();

        glm::vec3 point;
        for (size_t i = 0; i < 2; ++i) {
            point.x = i ? box.max.x : box.min.x;
            for (int j = 0; j < 2; ++j) {
                point.y = j ? box.max.y : box.min.y;
                for (int k = 0; k < 2; ++k) {
                    point.z = k ? box.max.z : box.min.z;
                    // change the point from model space to world space
                    point = modelMatrix * point;
                    result.min = glm::min(point, result.min);
                    result.max = glm::max(point, result.max);
                }
            }
        }

        return result;
    };

    BoundingBox result;
    for (size_t i = 0; i < m_bunnies.size(); ++i) {
        result += getModelBoundingBox(*m_bunnies[i]);
    }
    result += getModelBoundingBox(*m_ground);

    return result;
}

std::vector<float> ShadowMapping::getCascadeDistances() const {
    return std::vector<float>{m_camera->znear,        m_camera->zfar / 50.0f, m_camera->zfar / 25.0f,
                              m_camera->zfar / 10.0f, m_camera->zfar / 2.0f,  m_camera->zfar};
}