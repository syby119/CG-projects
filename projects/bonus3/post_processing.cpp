#include <random>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include "post_processing.h"

const std::string bunnyRelPath = "obj/bunny.obj";
const std::string cubeRelPath = "obj/cube.obj";
const std::string sphereRelPath = "obj/sphere.obj";

const std::string geometryVsRelPath = "shader/bonus3/geometry.vert";
const std::string geometryFsRelPath = "shader/bonus3/geometry.frag";

const std::string ssaoFsRelPath = "shader/bonus3/ssao.frag";
const std::string ssaoBlurFsRelPath = "shader/bonus3/ssao_blur.frag";
const std::string ssaoLightingFsRelPath = "shader/bonus3/ssao_lighting.frag";

const std::string lightVsRelPath = "shader/bonus3/light.vert";
const std::string lightFsRelPath = "shader/bonus3/light.frag";

const std::string brightColorFsRelPath = "shader/bonus3/extract_bright_color.frag";
const std::string gaussianBlurFsRelPath = "shader/bonus3/gaussian_blur.frag";
const std::string blendBloomMapFsRelPath = "shader/bonus3/blend_bloom_map.frag";

const std::string quadVsRelPath = "shader/bonus3/quad.vert";
const std::string quadFsRelPath = "shader/bonus3/quad.frag";

PostProcessing::PostProcessing(const Options& options) : Application(options) {
    m_bunny.reset(new Model(getAssetFullPath(bunnyRelPath)));
    m_bunny->transform.position = glm::vec3(0.0f, 2.5f, 0.0f);

    m_camera.reset(new PerspectiveCamera(
        glm::radians(60.0f), static_cast<float>(m_windowWidth) / m_windowHeight, 0.1f, 1000.0f));
    m_camera->transform.position = glm::vec3(4.0f, 6.0f, 10.0f);
    m_camera->transform.lookAt(glm::vec3(0.0f));

    m_pointLight.reset(new PointLight);
    m_pointLight->transform.position = glm::vec3(4.0f, 4.0f, 4.0f);
    m_pointLight->transform.scale = glm::vec3(0.3f, 0.3f, 0.3f);
    m_pointLight->color = glm::vec3(0.6f, 0.6f, 0.6f);
    m_pointLight->intensity = 3.0f;
    m_pointLight->kc = 1.0f;
    m_pointLight->kl = 0.0f;
    m_pointLight->kq = 0.05f;

    m_sphere.reset(new Model(getAssetFullPath(sphereRelPath)));

    m_cube.reset(new Model(getAssetFullPath(cubeRelPath)));
    m_cube->transform.position = glm::vec3(22.5659f, 25.1945f, 0.0f);
    m_cube->transform.scale = glm::vec3(50.0f);

    m_screenQuad.reset(new FullscreenQuad);

    initGeometryPassResources();

    initSSAOPassResources();

    initBloomPassResources();

    initShaders();

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
}

PostProcessing::~PostProcessing() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

void PostProcessing::handleInput() {
    if (m_input.keyboard.keyStates[GLFW_KEY_ESCAPE] != GLFW_RELEASE) {
        glfwSetWindowShouldClose(m_window, true);
        return;
    }
}

void PostProcessing::renderFrame() {
    renderScene();
    renderUI();
}

void PostProcessing::initGeometryPassResources() {
    // texture2d of (GL_RGB32F, GL_RGB, GL_FLOAT) is not renderable in WebGL2.0.
    // So we need to change the it to (GL_RGBA32F, GL_RGBA, GL_FLOAT) instead.
    constexpr GLint colorIFormat =
#ifdef USE_GLES
        GL_RGBA32F
#else
        GL_RGB32F
#endif
        ;

    constexpr GLenum colorFormat =
#ifdef USE_GLES
        GL_RGBA
#else
        GL_RGB
#endif
        ;

    m_gPosition.reset(
        new Texture2D(colorIFormat, m_windowWidth, m_windowHeight, colorFormat, GL_FLOAT));
    m_gPosition->bind();
    m_gPosition->setParamterInt(GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    m_gPosition->setParamterInt(GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    m_gPosition->setParamterInt(GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    m_gPosition->setParamterInt(GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    m_gPosition->unbind();

    m_gNormal.reset(
        new Texture2D(colorIFormat, m_windowWidth, m_windowHeight, colorFormat, GL_FLOAT));
    m_gNormal->bind();
    m_gNormal->setParamterInt(GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    m_gNormal->setParamterInt(GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    m_gNormal->setParamterInt(GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    m_gNormal->setParamterInt(GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    m_gNormal->unbind();

    m_gAlbedo.reset(
        new Texture2D(colorIFormat, m_windowWidth, m_windowHeight, colorFormat, GL_FLOAT));
    m_gAlbedo->bind();
    m_gAlbedo->setParamterInt(GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    m_gAlbedo->setParamterInt(GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    m_gAlbedo->setParamterInt(GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    m_gAlbedo->setParamterInt(GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    m_gNormal->unbind();

    constexpr GLint depthIFormat =
#ifdef USE_GLES
        GL_DEPTH_COMPONENT32F
#else
        GL_DEPTH_COMPONENT
#endif
        ;

    m_gDepth.reset(
        new Texture2D(depthIFormat, m_windowWidth, m_windowHeight, GL_DEPTH_COMPONENT, GL_FLOAT));

    m_gDepth->bind();
    m_gDepth->setParamterInt(GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    m_gDepth->setParamterInt(GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    m_gDepth->setParamterInt(GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    m_gDepth->setParamterInt(GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    m_gBufferFBO.reset(new Framebuffer);
    m_gBufferFBO->bind();
    m_gBufferFBO->drawBuffers({GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2});
    m_gBufferFBO->attachTexture2D(*m_gPosition, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D);
    m_gBufferFBO->attachTexture2D(*m_gNormal, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D);
    m_gBufferFBO->attachTexture2D(*m_gAlbedo, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D);
    m_gBufferFBO->attachTexture2D(*m_gDepth, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D);

    if (m_gBufferFBO->checkStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        throw std::runtime_error("m_gBufferFBO is imcomplete for rendering");
    }

    m_gBufferFBO->unbind();

    const char* version =
#ifdef USE_GLES
        "300 es"
#else
        "330 core"
#endif
        ;

    m_gBufferShader.reset(new GLSLProgram);
    m_gBufferShader->attachVertexShaderFromFile(getAssetFullPath(geometryVsRelPath), version);
    m_gBufferShader->attachFragmentShaderFromFile(getAssetFullPath(geometryFsRelPath), version);
    m_gBufferShader->link();
}

void PostProcessing::initSSAOPassResources() {
    m_ssaoFBO.reset(new Framebuffer);
    m_ssaoFBO->bind();
    m_ssaoFBO->drawBuffer(GL_COLOR_ATTACHMENT0);
    for (int i = 0; i < 2; ++i) {
        m_ssaoResult[i].reset(
            new Texture2D(GL_R32F, m_windowWidth, m_windowHeight, GL_RED, GL_FLOAT));
        m_ssaoResult[i]->bind();
        m_ssaoResult[i]->setParamterInt(GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        m_ssaoResult[i]->setParamterInt(GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        m_ssaoResult[i]->setParamterInt(GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        m_ssaoResult[i]->setParamterInt(GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    }
    m_ssaoFBO->attachTexture2D(*m_ssaoResult[0], GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D);

    if (m_ssaoFBO->checkStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        throw std::runtime_error("m_ssaoFBO is imcomplete for rendering");
    }

    m_ssaoFBO->unbind();

    m_ssaoBlurFBO.reset(new Framebuffer);
    m_ssaoBlurFBO->bind();
    m_ssaoBlurFBO->drawBuffer(GL_COLOR_ATTACHMENT0);
    m_ssaoBlurFBO->unbind();

    std::default_random_engine e;
    std::uniform_real_distribution<float> u(0.0f, 1.0f);

    for (unsigned int i = 0; i < 64; ++i) {
        glm::vec3 sample(u(e) * 2.0f - 1.0f, u(e) * 2.0f - 1.0f, u(e));
        sample = glm::normalize(sample);
        sample *= u(e);
        float scale = float(i) / 64.0f;

        // scale samples s.t. they're more aligned to center of kernel
        scale = glm::mix(0.1f, 1.0f, scale * scale);
        sample *= scale;
        m_sampleVecs.push_back(sample);
    }

    std::vector<glm::vec3> ssaoNoises;
    for (unsigned int i = 0; i < 16; i++) {
        // rotate around z-axis (in tangent space)
        glm::vec3 noise(u(e) * 2.0f - 1.0f, u(e) * 2.0f - 1.0f, 0.0f);
        ssaoNoises.push_back(noise);
    }

    m_ssaoNoise.reset(new Texture2D(GL_RGB32F, 4, 4, GL_RGB, GL_FLOAT, ssaoNoises.data()));
    m_ssaoNoise->bind();
    m_ssaoNoise->setParamterInt(GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    m_ssaoNoise->setParamterInt(GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    m_ssaoNoise->setParamterInt(GL_TEXTURE_WRAP_S, GL_REPEAT);
    m_ssaoNoise->setParamterInt(GL_TEXTURE_WRAP_T, GL_REPEAT);
    m_ssaoNoise->unbind();

    const char* version =
#ifdef USE_GLES
        "300 es"
#else
        "330 core"
#endif
        ;

    // TODO: modify ssao.frag
    m_ssaoShader.reset(new GLSLProgram);
    m_ssaoShader->attachVertexShaderFromFile(getAssetFullPath(quadVsRelPath), version);
    m_ssaoShader->attachFragmentShaderFromFile(getAssetFullPath(ssaoFsRelPath), version);
    m_ssaoShader->link();

    m_ssaoBlurShader.reset(new GLSLProgram);
    m_ssaoBlurShader->attachVertexShaderFromFile(getAssetFullPath(quadVsRelPath), version);
    m_ssaoBlurShader->attachFragmentShaderFromFile(getAssetFullPath(ssaoBlurFsRelPath), version);
    m_ssaoBlurShader->link();

    // TODO: modify ssao_lighting.frag
    m_ssaoLightingShader.reset(new GLSLProgram);
    m_ssaoLightingShader->attachVertexShaderFromFile(getAssetFullPath(quadVsRelPath), version);
    m_ssaoLightingShader->attachFragmentShaderFromFile(
        getAssetFullPath(ssaoLightingFsRelPath), version);
    m_ssaoLightingShader->link();
}

void PostProcessing::initBloomPassResources() {
    m_bloomFBO.reset(new Framebuffer);
    m_bloomFBO->bind();
    m_bloomFBO->drawBuffer(GL_COLOR_ATTACHMENT0);

    m_bloomMap.reset(new Texture2D(GL_RGBA32F, m_windowWidth, m_windowHeight, GL_RGBA, GL_FLOAT));
    m_bloomMap->bind();
    m_bloomMap->setParamterInt(GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    m_bloomMap->setParamterInt(GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    m_bloomMap->setParamterInt(GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    m_bloomMap->setParamterInt(GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    m_bloomFBO->attachTexture2D(*m_bloomMap, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D);
    for (int i = 0; i < 2; ++i) {
        m_brightColorMap[i].reset(
            new Texture2D(GL_RGBA32F, m_windowWidth, m_windowHeight, GL_RGBA, GL_FLOAT));
        m_brightColorMap[i]->bind();
        m_brightColorMap[i]->setParamterInt(GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        m_brightColorMap[i]->setParamterInt(GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        m_brightColorMap[i]->setParamterInt(GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        m_brightColorMap[i]->setParamterInt(GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    }

    m_bloomFBO->attachTexture2D(*m_gDepth, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D);

    if (m_bloomFBO->checkStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        throw std::runtime_error("m_bloomFBO is imcomplete for rendering");
    }

    m_bloomFBO->unbind();

    m_brightColorFBO.reset(new Framebuffer);
    m_brightColorFBO->bind();
    m_brightColorFBO->drawBuffer(GL_COLOR_ATTACHMENT0);
    m_brightColorFBO->unbind();

    m_blurFBO.reset(new Framebuffer);
    m_blurFBO->bind();
    m_blurFBO->drawBuffer(GL_COLOR_ATTACHMENT0);
    m_blurFBO->unbind();

    const char* version =
#ifdef USE_GLES
        "300 es"
#else
        "330 core"
#endif
        ;

    m_lightShader.reset(new GLSLProgram);
    m_lightShader->attachVertexShaderFromFile(getAssetFullPath(lightVsRelPath), version);
    m_lightShader->attachFragmentShaderFromFile(getAssetFullPath(lightFsRelPath), version);
    m_lightShader->link();

    // TODO: modify extract_bright_color.frag
    m_brightColorShader.reset(new GLSLProgram);
    m_brightColorShader->attachVertexShaderFromFile(getAssetFullPath(quadVsRelPath), version);
    m_brightColorShader->attachFragmentShaderFromFile(
        getAssetFullPath(brightColorFsRelPath), version);
    m_brightColorShader->link();

    // TODO: modify gaussian_blur.frag
    m_blurShader.reset(new GLSLProgram);
    m_blurShader->attachVertexShaderFromFile(getAssetFullPath(quadVsRelPath), version);
    m_blurShader->attachFragmentShaderFromFile(getAssetFullPath(gaussianBlurFsRelPath), version);
    m_blurShader->link();

    m_blendShader.reset(new GLSLProgram);
    m_blendShader->attachVertexShaderFromFile(getAssetFullPath(quadVsRelPath), version);
    m_blendShader->attachFragmentShaderFromFile(getAssetFullPath(blendBloomMapFsRelPath), version);
    m_blendShader->link();
}

void PostProcessing::initShaders() {
    const char* version =
#ifdef USE_GLES
        "300 es"
#else
        "330 core"
#endif
        ;

    m_drawScreenShader.reset(new GLSLProgram);
    m_drawScreenShader->attachVertexShaderFromFile(getAssetFullPath(quadVsRelPath), version);
    m_drawScreenShader->attachFragmentShaderFromFile(getAssetFullPath(quadFsRelPath), version);
    m_drawScreenShader->link();
}

void PostProcessing::renderScene() {
    glClearColor(m_clearColor.r, m_clearColor.g, m_clearColor.b, m_clearColor.a);

    // deferred rendering: geometry pass
    m_gBufferFBO->bind();
    glEnable(GL_DEPTH_TEST);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    m_gBufferShader->use();
    m_gBufferShader->setUniformMat4("projection", m_camera->getProjectionMatrix());
    m_gBufferShader->setUniformMat4("view", m_camera->getViewMatrix());

    m_gBufferShader->setUniformMat4("model", m_bunny->transform.getLocalMatrix());
    m_bunny->draw();

    m_gBufferShader->setUniformMat4("model", m_cube->transform.getLocalMatrix());
    m_cube->draw();

    m_gBufferFBO->unbind();

    // deferred rendering: lighting passes
    // + SSAO pass
    if (m_enableSSAO) {
        glDisable(GL_DEPTH_TEST);

        m_ssaoFBO->bind();

        m_ssaoShader->use();
        m_ssaoShader->setUniformInt("gPosition", 0);
        m_gPosition->bind(0);
        m_ssaoShader->setUniformInt("gNormal", 1);
        m_gNormal->bind(1);
        m_ssaoShader->setUniformInt("noiseMap", 2);
        m_ssaoNoise->bind(2);
        for (size_t i = 0; i < m_sampleVecs.size(); ++i) {
            m_ssaoShader->setUniformVec3("sampleVecs[" + std::to_string(i) + "]", m_sampleVecs[i]);
        }

        m_ssaoShader->setUniformInt("screenWidth", m_windowWidth);
        m_ssaoShader->setUniformInt("screenHeight", m_windowHeight);
        m_ssaoShader->setUniformMat4("projection", m_camera->getProjectionMatrix());
        m_screenQuad->draw();

        m_ssaoFBO->unbind();

        m_ssaoBlurFBO->bind();

        m_currentReadBuffer = 0;
        m_currentWriteBuffer = 1;
        m_ssaoBlurShader->use();
        for (int pass = 0; pass < 5; ++pass) {
            m_ssaoBlurFBO->attachTexture2D(
                *m_ssaoResult[m_currentWriteBuffer], GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D);
            m_ssaoBlurShader->setUniformInt("ssaoResult", 0);
            m_ssaoResult[m_currentReadBuffer]->bind(0);
            m_screenQuad->draw();

            std::swap(m_currentReadBuffer, m_currentWriteBuffer);
        }

        m_ssaoBlurFBO->unbind();
    } else {
        m_currentReadBuffer = 0;
        static const std::vector<float> ones(m_windowWidth * m_windowHeight, 1.0f);
        m_ssaoResult[0]->bind();
        glTexImage2D(
            GL_TEXTURE_2D, 0, GL_R32F, m_windowWidth, m_windowHeight, 0, GL_RED, GL_FLOAT,
            ones.data());
        m_ssaoResult[0]->unbind();
    }

    // + bloom pass
    m_bloomFBO->bind();
    glDisable(GL_DEPTH_TEST);
    glClear(GL_COLOR_BUFFER_BIT);

    m_ssaoLightingShader->use();

    m_ssaoLightingShader->setUniformVec3("light.position", m_pointLight->transform.position);
    m_ssaoLightingShader->setUniformVec3("light.color", m_pointLight->color);
    m_ssaoLightingShader->setUniformFloat("light.intensity", m_pointLight->intensity);
    m_ssaoLightingShader->setUniformFloat("light.kc", m_pointLight->kc);
    m_ssaoLightingShader->setUniformFloat("light.kl", m_pointLight->kl);
    m_ssaoLightingShader->setUniformFloat("light.kq", m_pointLight->kq);

    m_ssaoLightingShader->setUniformInt("gPosition", 0);
    m_gPosition->bind(0);
    m_ssaoLightingShader->setUniformInt("gNormal", 1);
    m_gNormal->bind(1);
    m_ssaoLightingShader->setUniformInt("gAlbedo", 2);
    m_gAlbedo->bind(2);
    m_ssaoLightingShader->setUniformInt("ssaoResult", 3);
    m_ssaoResult[m_currentReadBuffer]->bind(3);

    m_screenQuad->draw();

    glEnable(GL_DEPTH_TEST);
    m_lightShader->use();

    m_lightShader->setUniformMat4("projection", m_camera->getProjectionMatrix());
    m_lightShader->setUniformMat4("view", m_camera->getViewMatrix());
    m_lightShader->setUniformMat4("model", m_pointLight->transform.getLocalMatrix());
    m_lightShader->setUniformVec3("lightColor", m_pointLight->color);
    m_lightShader->setUniformFloat("lightIntensity", m_pointLight->intensity);

    m_sphere->draw();

    m_bloomFBO->unbind();

    if (m_enableBloom) {
        extractBrightColor(*m_bloomMap);
        blurBrightColor();
        combineSceneMapAndBloomBlur(*m_bloomMap);
    } else {
        glDisable(GL_DEPTH_TEST);
        m_drawScreenShader->use();
        m_drawScreenShader->setUniformInt("frame", 0);
        m_bloomMap->bind(0);
        m_screenQuad->draw();
    }
}

void PostProcessing::renderUI() {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    const auto flags = ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings;

    if (!ImGui::Begin("Control Panel", nullptr, flags)) {
        ImGui::End();
    } else {
        ImGui::Text("post processing technics");
        ImGui::Separator();
        ImGui::Checkbox("bloom", &m_enableBloom);
        ImGui::Checkbox("ssao", &m_enableSSAO);
        ImGui::End();
    }

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void PostProcessing::extractBrightColor(const Texture2D& sceneMap) {
    m_brightColorFBO->bind();
    m_brightColorFBO->attachTexture2D(*m_brightColorMap[0], GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D);
    m_brightColorShader->use();
    m_brightColorShader->setUniformInt("sceneMap", 0);
    sceneMap.bind(0);
    m_screenQuad->draw();
    m_brightColorFBO->unbind();
}

void PostProcessing::blurBrightColor() {
    m_blurFBO->bind();
    m_blurFBO->drawBuffer(GL_COLOR_ATTACHMENT0);
    m_blurShader->use();
    bool horizontal = true;
    m_blurShader->setUniformInt("image", 0);
    m_currentReadBuffer = 0;
    m_currentWriteBuffer = 1;

    for (int pass = 0; pass < 20; ++pass) {
        m_blurShader->setUniformBool("horizontal", horizontal);
        m_blurFBO->attachTexture2D(
            *m_brightColorMap[m_currentWriteBuffer], GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D);
        m_brightColorMap[m_currentReadBuffer]->bind(0);
        m_screenQuad->draw();
        horizontal = !horizontal;
        std::swap(m_currentReadBuffer, m_currentWriteBuffer);
    }

    m_blurFBO->unbind();
}

void PostProcessing::combineSceneMapAndBloomBlur(const Texture2D& sceneMap) {
    glDisable(GL_DEPTH_TEST);
    m_blendShader->use();

    m_blendShader->setUniformInt("scene", 0);
    sceneMap.bind(0);

    m_blendShader->setUniformInt("bloomBlur", 1);
    m_brightColorMap[m_currentReadBuffer]->bind(1);

    m_screenQuad->draw();
}