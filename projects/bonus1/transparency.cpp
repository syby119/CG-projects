#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include "transparency.h"

const std::string knotRelPath = "obj/knot.obj";
const std::string transparentTextureRelPath = "texture/miscellaneous/transparent.png";

const std::string alphaTestingVsRelPath = "shader/bonus1/model.vert";
const std::string alphaTestingFsRelPath = "shader/bonus1/alpha_test.frag";

const std::string alphaBlendingVsRelPath = "shader/bonus1/model.vert";
const std::string alphaBlendingFsRelPath = "shader/bonus1/alpha_blend.frag";

const std::string oitInitVsRelPath = "shader/bonus1/model.vert";
const std::string oitInitFsRelPath = "shader/bonus1/oit_init.frag";

const std::string oitPeelVsRelPath = "shader/bonus1/model.vert";
const std::string oitPeelFsRelPath = "shader/bonus1/oit_peel.frag";

const std::string oitBlendVsRelPath = "shader/bonus1/quad.vert";
const std::string oitBlendFsRelPath = "shader/bonus1/oit_blend.frag";

const std::string oitFinalVsRelPath = "shader/bonus1/quad.vert";
const std::string oitFinalFsRelPath = "shader/bonus1/oit_final.frag";

Transparency::Transparency(const Options& options) : Application(options) {
    // init models
    m_knot.reset(new Model(getAssetFullPath(knotRelPath)));
    m_knot->transform.scale = glm::vec3(0.8f, 0.8f, 0.8f);

    // init light
    m_light.reset(new DirectionalLight());
    m_light->transform.rotation =
        glm::angleAxis(glm::radians(45.0f), glm::normalize(glm::vec3(-1.0f)));

    // init camera
    m_camera.reset(new PerspectiveCamera(
        glm::radians(50.0f), 1.0f * m_windowWidth / m_windowHeight, 0.1f, 10000.0f));
    m_camera->transform.position.z = 10.0f;

    // init shaders
    initShaders();

    // init materials
    m_knotMaterial.reset(new TransparentMaterial());
    m_knotMaterial->albedo = glm::vec3(1.0f, 1.0f, 1.0f);
    m_knotMaterial->ka = 0.03f;
    m_knotMaterial->kd = glm::vec3(1.0f, 1.0f, 1.0f);
    m_knotMaterial->transparent = 0.8f;

    // init sphere texture
    m_transparentTexture.reset(new ImageTexture2D(getAssetFullPath(transparentTextureRelPath)));

    // init fullscreen quad
    m_fullscreenQuad.reset(new FullscreenQuad);

    // init depth peeling resources
    initDepthPeelingResources();

    // init query
    glGenQueries(1, &m_queryId);

    // init imGUI
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    (void)io;

    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(m_window, true);
    ImGui_ImplOpenGL3_Init();
}

Transparency::~Transparency() {
    if (m_queryId) {
        glDeleteQueries(1, &m_queryId);
        m_queryId = 0;
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

void Transparency::initShaders() {
    // alpha testing shader
    // TODO: modify the alpha_testing.frag code to achieve the alpha testing algorithm
    m_alphaTestingShader.reset(new GLSLProgram);
    m_alphaTestingShader->attachVertexShaderFromFile(getAssetFullPath(alphaTestingVsRelPath));
    m_alphaTestingShader->attachFragmentShaderFromFile(getAssetFullPath(alphaTestingFsRelPath));
    m_alphaTestingShader->link();

    // alpha blending shader
    m_alphaBlendingShader.reset(new GLSLProgram);
    m_alphaBlendingShader->attachVertexShaderFromFile(getAssetFullPath(alphaBlendingVsRelPath));
    m_alphaBlendingShader->attachFragmentShaderFromFile(getAssetFullPath(alphaBlendingFsRelPath));
    m_alphaBlendingShader->link();

    // depth peeling shaders
    m_depthPeelingInitShader.reset(new GLSLProgram);
    m_depthPeelingInitShader->attachVertexShaderFromFile(getAssetFullPath(oitInitVsRelPath));
    m_depthPeelingInitShader->attachFragmentShaderFromFile(getAssetFullPath(oitInitFsRelPath));
    m_depthPeelingInitShader->link();

    m_depthPeelingShader.reset(new GLSLProgram);
    m_depthPeelingShader->attachVertexShaderFromFile(getAssetFullPath(oitPeelVsRelPath));
    m_depthPeelingShader->attachFragmentShaderFromFile(getAssetFullPath(oitPeelFsRelPath));
    m_depthPeelingShader->link();

    m_depthPeelingBlendShader.reset(new GLSLProgram);
    m_depthPeelingBlendShader->attachVertexShaderFromFile(getAssetFullPath(oitBlendVsRelPath));
    m_depthPeelingBlendShader->attachFragmentShaderFromFile(getAssetFullPath(oitBlendFsRelPath));
    m_depthPeelingBlendShader->link();

    m_depthPeelingFinalShader.reset(new GLSLProgram);
    m_depthPeelingFinalShader->attachVertexShaderFromFile(getAssetFullPath(oitFinalVsRelPath));
    m_depthPeelingFinalShader->attachFragmentShaderFromFile(getAssetFullPath(oitFinalFsRelPath));
    m_depthPeelingFinalShader->link();
}

void Transparency::initDepthPeelingResources() {
    // ping-pong framebuffers
    for (int i = 0; i < 2; ++i) {
        m_fbos[i].reset(new Framebuffer);

        m_colorTextures[i].reset(
            new Texture2D(GL_RGBA32F, m_windowWidth, m_windowHeight, GL_RGBA, GL_FLOAT));

        m_depthTextures[i].reset(new Texture2D(
            GL_DEPTH_COMPONENT, m_windowWidth, m_windowHeight, GL_DEPTH_COMPONENT, GL_FLOAT));

        m_fbos[i]->bind();
        m_fbos[i]->attachTexture2D(*m_colorTextures[i], GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D);
        m_fbos[i]->attachTexture2D(*m_depthTextures[i], GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D);

        GLenum status = m_fbos[i]->checkStatus();
        if (status != GL_FRAMEBUFFER_COMPLETE) {
            throw std::runtime_error(m_fbos[i]->getDiagnostic(status));
        }

        m_fbos[i]->unbind();
    }

    // blend framebuffer
    m_colorBlendFbo.reset(new Framebuffer);

    m_colorBlendTexture.reset(
        new Texture2D(GL_RGBA32F, m_windowWidth, m_windowHeight, GL_RGBA, GL_FLOAT));

    m_colorBlendFbo->bind();
    m_colorBlendFbo->attachTexture2D(*m_colorBlendTexture, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D);
    m_colorBlendFbo->attachTexture2D(*m_depthTextures[0], GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D);

    GLenum status = m_colorBlendFbo->checkStatus();
    if (status != GL_FRAMEBUFFER_COMPLETE) {
        throw std::runtime_error(m_colorBlendFbo->getDiagnostic(status));
    }

    m_colorBlendFbo->unbind();

    checkGLErrors();
}

void Transparency::handleInput() {
    if (m_input.keyboard.keyStates[GLFW_KEY_ESCAPE] != GLFW_RELEASE) {
        glfwSetWindowShouldClose(m_window, true);
        return;
    }

    const float angluarVelocity = 0.1f;
    const float angle = angluarVelocity * static_cast<float>(m_deltaTime);
    const glm::vec3 axis = glm::vec3(0.0f, 1.0f, 0.0f);
    m_knot->transform.rotation = glm::angleAxis(angle, axis) * m_knot->transform.rotation;
}

void Transparency::renderFrame() {
    // trivial things
    showFpsInWindowTitle();

    glClearColor(m_clearColor.r, m_clearColor.g, m_clearColor.b, m_clearColor.a);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);

    switch (m_renderMode) {
    case RenderMode::AlphaTesting: renderWithAlphaTesting(); break;
    case RenderMode::AlphaBlending: renderWithAlphaBlending(); break;
    case RenderMode::DepthPeeling: renderWithDepthPeeling(); break;
    }

    // draw ui elements
    renderUI();
}

void Transparency::renderWithAlphaTesting() {
    m_alphaTestingShader->use();
    // 1 set transformation matrices
    m_alphaTestingShader->setUniformMat4("projection", m_camera->getProjectionMatrix());
    m_alphaTestingShader->setUniformMat4("view", m_camera->getViewMatrix());
    m_alphaTestingShader->setUniformMat4("model", m_knot->transform.getLocalMatrix());
    // 2 set light
    m_alphaTestingShader->setUniformVec3("directionalLight.direction", m_light->transform.getFront());
    m_alphaTestingShader->setUniformFloat("directionalLight.intensity", m_light->intensity);
    m_alphaTestingShader->setUniformVec3("directionalLight.color", m_light->color);
    // 3 set material
    m_alphaTestingShader->setUniformVec3("material.albedo", m_knotMaterial->albedo);
    m_alphaTestingShader->setUniformFloat("material.ka", m_knotMaterial->ka);
    m_alphaTestingShader->setUniformVec3("material.kd", m_knotMaterial->kd);
    m_alphaTestingShader->setUniformFloat("material.transparent", m_knotMaterial->transparent);
    // 4 set texture
    m_transparentTexture->bind(0);

    m_knot->draw();
}

void Transparency::renderWithAlphaBlending() {
    //  render transparent objects
    m_alphaBlendingShader->use();
    // 1 set transformation matrices
    m_alphaBlendingShader->setUniformMat4("projection", m_camera->getProjectionMatrix());
    m_alphaBlendingShader->setUniformMat4("view", m_camera->getViewMatrix());
    m_alphaBlendingShader->setUniformMat4("model", m_knot->transform.getLocalMatrix());
    // 2 set light
    m_alphaBlendingShader->setUniformVec3(
        "directionalLight.direction", m_light->transform.getFront());
    m_alphaBlendingShader->setUniformFloat("directionalLight.intensity", m_light->intensity);
    m_alphaBlendingShader->setUniformVec3("directionalLight.color", m_light->color);
    // 3 set material
    m_alphaBlendingShader->setUniformVec3("material.albedo", m_knotMaterial->albedo);
    m_alphaBlendingShader->setUniformFloat("material.ka", m_knotMaterial->ka);
    m_alphaBlendingShader->setUniformVec3("material.kd", m_knotMaterial->kd);
    m_alphaBlendingShader->setUniformFloat("material.transparent", m_knotMaterial->transparent);

    // TODO: use two render passes to achieve alpha blending
    // pass 1: Write the depth info to the zbuffer, while leave the color buffer unmodified.
    //         This pass will record the depth info of the object to avoid backward parts to
    //         be rendered.
    // write your code here
    // ------------------------------------------------------------------------
    // ...
    // ------------------------------------------------------------------------

    m_knot->draw();

    // pass 2: Write the color buffer using the zbuffer info from pass 1 with blending,
    //           while leaving the depth buffer unmodified.
    // write your code here
    // ------------------------------------------------------------------------
    // ...
    // ------------------------------------------------------------------------

    m_knot->draw();
    // restore: don't forget to restore the OpenGL state before pass 1, which will avoid side
    // effects
    //          to the object rendering afterwards.
    // write your code here
    // ------------------------------------------------------------------------
    // ...
    // ------------------------------------------------------------------------
}

void Transparency::renderWithDepthPeeling() {
    const glm::mat4 projection = m_camera->getProjectionMatrix();
    const glm::mat4 view = m_camera->getViewMatrix();

    // 1. initialize min depth buffer
    m_colorBlendFbo->bind();

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);

    m_depthPeelingInitShader->use();
    // 1.1 set transformation matrices
    m_depthPeelingInitShader->setUniformMat4("projection", projection);
    m_depthPeelingInitShader->setUniformMat4("view", view);
    m_depthPeelingInitShader->setUniformMat4("model", m_knot->transform.getLocalMatrix());
    // 1.2 set light
    m_depthPeelingInitShader->setUniformVec3(
        "directionalLight.direction", m_light->transform.getFront());
    m_depthPeelingInitShader->setUniformFloat("directionalLight.intensity", m_light->intensity);
    m_depthPeelingInitShader->setUniformVec3("directionalLight.color", m_light->color);
    // 1.3 set material
    m_depthPeelingInitShader->setUniformVec3("material.albedo", m_knotMaterial->albedo);
    m_depthPeelingInitShader->setUniformFloat("material.ka", m_knotMaterial->ka);
    m_depthPeelingInitShader->setUniformVec3("material.kd", m_knotMaterial->kd);
    m_depthPeelingInitShader->setUniformFloat("material.transparent", m_knotMaterial->transparent);

    m_knot->draw();

    // 2. TODO: depth peeling and blending
    // hint1: this stage can be divided into iterative 2 pass: peeling pass and blending pass
    // hint2: use m_fbos as ping-pong framebuffer for peeling pass
    // hint3: use m_depthPeelingShader for peeling pass
    // hint4: use m_colorBlendFbo for blending pass
    // hint5: use m_depthPeelingBlendShader for blend pass
    // hint6: you can use glBeginQuery / glEndQuery / glGetQueryObjectuiv to end looping.
    // hint7: if it is to difficult for you, just use a predefined MAX_LAYER_NUM to end looping
    // write your code here
    // ------------------------------------------------------------------------
    // for (int layer = 1; layer < MAX_LAYER_NUM; ++layer) {
    //        // 2.1 peeling pass
    //        // 2.2 blending pass
    // }
    // ------------------------------------------------------------------------

    // 3. final pass: blend the peeling result with the background color
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glDisable(GL_DEPTH_TEST);
    m_depthPeelingFinalShader->use();
    // 3.1 set the window extent
    m_depthPeelingFinalShader->setUniformInt("windowExtent.width", m_windowWidth);
    m_depthPeelingFinalShader->setUniformInt("windowExtent.height", m_windowHeight);
    // 3.2 set the blend texture
    m_colorBlendTexture->bind(0);
    // 3.3 set the background color
    m_depthPeelingFinalShader->setUniformVec4("backgroundColor", m_clearColor);

    m_fullscreenQuad->draw();
}

void Transparency::renderUI() {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    const auto flags = ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings;

    if (!ImGui::Begin("Control Panel", nullptr, flags)) {
        ImGui::End();
    } else {
        ImGui::Text("Render Mode");
        ImGui::Separator();
        ImGui::RadioButton("Alpha Testing", (int*)&m_renderMode, (int)(RenderMode::AlphaTesting));
        ImGui::RadioButton("Alpha Blending", (int*)&m_renderMode, (int)(RenderMode::AlphaBlending));
        ImGui::RadioButton("Depth Peeling", (int*)&m_renderMode, (int)(RenderMode::DepthPeeling));
        ImGui::SliderFloat("transparent", &m_knotMaterial->transparent, 0.0f, 1.0f);
        ImGui::NewLine();

        ImGui::ColorEdit3("background", (float*)&m_clearColor);

        ImGui::End();
    }

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}