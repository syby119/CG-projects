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

Transparency::Transparency(const Options& options): Application(options) {
    // init models
    _knot.reset(new Model(getAssetFullPath(knotRelPath)));
    _knot->transform.scale = glm::vec3(0.8f, 0.8f, 0.8f);

    // init light
    _light.reset(new DirectionalLight());
    _light->transform.rotation = glm::angleAxis(glm::radians(45.0f), glm::normalize(glm::vec3(-1.0f)));

    // init camera
    _camera.reset(new PerspectiveCamera(
        glm::radians(50.0f), 1.0f * _windowWidth / _windowHeight, 0.1f, 10000.0f));
    _camera->transform.position.z = 10.0f;

    // init shaders
    initShaders();

    // init materials
    _knotMaterial.reset(new TransparentMaterial());
    _knotMaterial->albedo = glm::vec3(1.0f, 1.0f, 1.0f);
    _knotMaterial->ka = 0.03f;
    _knotMaterial->kd = glm::vec3(1.0f, 1.0f, 1.0f);
    _knotMaterial->transparent = 0.8f;

    // init sphere texture
    _transparentTexture.reset(new ImageTexture2D(getAssetFullPath(transparentTextureRelPath)));

    // init fullscreen quad
    _fullscreenQuad.reset(new FullscreenQuad);

    // init depth peeling resources
    initDepthPeelingResources();

    // init query
    glGenQueries(1, &_queryId);

    // init imGUI
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;

    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(_window, true);
#if defined(__EMSCRIPTEN__)
    ImGui_ImplOpenGL3_Init("#version 100");
#elif defined(USE_GLES)
    ImGui_ImplOpenGL3_Init("#version 150");
#else
    ImGui_ImplOpenGL3_Init();
#endif
}

Transparency::~Transparency() {
    if (_queryId) {
        glDeleteQueries(1, &_queryId);
        _queryId = 0;
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

void Transparency::initShaders() {
    const char* version =
#ifdef USE_GLES
        "300 es"
#else
        "330 core"
#endif
        ;

    // alpha testing shader
    // TODO: modify the alpha_testing.frag code to achieve the alpha testing algorithm
    _alphaTestingShader.reset(new GLSLProgram);
    _alphaTestingShader->attachVertexShaderFromFile(
        getAssetFullPath(alphaTestingVsRelPath), version);
    _alphaTestingShader->attachFragmentShaderFromFile(
        getAssetFullPath(alphaTestingFsRelPath), version);
    _alphaTestingShader->link();

    // alpha blending shader
    _alphaBlendingShader.reset(new GLSLProgram);
    _alphaBlendingShader->attachVertexShaderFromFile(
        getAssetFullPath(alphaBlendingVsRelPath), version);
    _alphaBlendingShader->attachFragmentShaderFromFile(
        getAssetFullPath(alphaBlendingFsRelPath), version);
    _alphaBlendingShader->link();

    // depth peeling shaders
    _depthPeelingInitShader.reset(new GLSLProgram);
    _depthPeelingInitShader->attachVertexShaderFromFile(
        getAssetFullPath(oitInitVsRelPath), version);
    _depthPeelingInitShader->attachFragmentShaderFromFile(
        getAssetFullPath(oitInitFsRelPath), version);
    _depthPeelingInitShader->link();

    _depthPeelingShader.reset(new GLSLProgram);
    _depthPeelingShader->attachVertexShaderFromFile(
        getAssetFullPath(oitPeelVsRelPath), version);
    _depthPeelingShader->attachFragmentShaderFromFile(
        getAssetFullPath(oitPeelFsRelPath), version);
    _depthPeelingShader->link();

    _depthPeelingBlendShader.reset(new GLSLProgram);
    _depthPeelingBlendShader->attachVertexShaderFromFile(
        getAssetFullPath(oitBlendVsRelPath), version);
    _depthPeelingBlendShader->attachFragmentShaderFromFile(
        getAssetFullPath(oitBlendFsRelPath), version);
    _depthPeelingBlendShader->link();

    _depthPeelingFinalShader.reset(new GLSLProgram);
    _depthPeelingFinalShader->attachVertexShaderFromFile(
        getAssetFullPath(oitFinalVsRelPath), version);
    _depthPeelingFinalShader->attachFragmentShaderFromFile(
        getAssetFullPath(oitFinalFsRelPath), version);
    _depthPeelingFinalShader->link();
}

void Transparency::initDepthPeelingResources() {
    // ping-pong framebuffers
    for (int i = 0; i < 2; ++i) {
        _fbos[i].reset(new Framebuffer);

        _colorTextures[i].reset(new Texture2D(
            GL_RGBA32F, _windowWidth, _windowHeight, GL_RGBA, GL_FLOAT));

#ifdef USE_GLES
        _depthTextures[i].reset(new Texture2D(
            GL_DEPTH_COMPONENT32F, _windowWidth, _windowHeight, GL_DEPTH_COMPONENT, GL_FLOAT));
#else
        _depthTextures[i].reset(new Texture2D(
            GL_DEPTH_COMPONENT, _windowWidth, _windowHeight, GL_DEPTH_COMPONENT, GL_FLOAT));
#endif

        _fbos[i]->bind();
        _fbos[i]->attachTexture2D(*_colorTextures[i], GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D);
        _fbos[i]->attachTexture2D(*_depthTextures[i], GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D);
        
        GLenum status = _fbos[i]->checkStatus();
        if (status != GL_FRAMEBUFFER_COMPLETE) {
            throw std::runtime_error(_fbos[i]->getDiagnostic(status));
        }

        _fbos[i]->unbind();
    }

    // blend framebuffer
    _colorBlendFbo.reset(new Framebuffer);

    _colorBlendTexture.reset(new Texture2D(
        GL_RGBA32F, _windowWidth, _windowHeight, GL_RGBA, GL_FLOAT));

    _colorBlendFbo->bind();
    _colorBlendFbo->attachTexture2D(*_colorBlendTexture, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D);
    _colorBlendFbo->attachTexture2D(*_depthTextures[0], GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D);

    GLenum status = _colorBlendFbo->checkStatus();
    if (status != GL_FRAMEBUFFER_COMPLETE) {
        throw std::runtime_error(_colorBlendFbo->getDiagnostic(status));
    }
    
    _colorBlendFbo->unbind();

    checkGLErrors();
}

void Transparency::handleInput() {
    if (_input.keyboard.keyStates[GLFW_KEY_ESCAPE] != GLFW_RELEASE) {
        glfwSetWindowShouldClose(_window, true);
        return ;
    }
    
    const float angluarVelocity = 0.1f;
    const float angle = angluarVelocity * static_cast<float>(_deltaTime);
    const glm::vec3 axis = glm::vec3(0.0f, 1.0f, 0.0f);
    _knot->transform.rotation = glm::angleAxis(angle, axis) * _knot->transform.rotation;
}

void Transparency::renderFrame() {
    // trivial things
    showFpsInWindowTitle();

    glClearColor(_clearColor.r, _clearColor.g, _clearColor.b, _clearColor.a);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);

    switch (_renderMode) {
    case RenderMode::AlphaTesting:
        renderWithAlphaTesting();
        break;
    case RenderMode::AlphaBlending:
        renderWithAlphaBlending();
        break;
    case RenderMode::DepthPeeling:
        renderWithDepthPeeling();
        break;
    }

    // draw ui elements
    renderUI();
}

void Transparency::renderWithAlphaTesting() {
    _alphaTestingShader->use();
    // 1 set transformation matrices
    _alphaTestingShader->setUniformMat4("projection", _camera->getProjectionMatrix());
    _alphaTestingShader->setUniformMat4("view", _camera->getViewMatrix());
    _alphaTestingShader->setUniformMat4("model", _knot->transform.getLocalMatrix());
    // 2 set light
    _alphaTestingShader->setUniformVec3("directionalLight.direction", _light->transform.getFront());
    _alphaTestingShader->setUniformFloat("directionalLight.intensity", _light->intensity);
    _alphaTestingShader->setUniformVec3("directionalLight.color", _light->color);
    // 3 set material
    _alphaTestingShader->setUniformVec3("material.albedo", _knotMaterial->albedo);
    _alphaTestingShader->setUniformFloat("material.ka", _knotMaterial->ka);
    _alphaTestingShader->setUniformVec3("material.kd", _knotMaterial->kd);
    _alphaTestingShader->setUniformFloat("material.transparent", _knotMaterial->transparent);
    // 4 set texture
    _transparentTexture->bind(0);

    _knot->draw();
}

void Transparency::renderWithAlphaBlending() {
    //  render transparent objects
    _alphaBlendingShader->use();
    // 1 set transformation matrices
    _alphaBlendingShader->setUniformMat4("projection", _camera->getProjectionMatrix());
    _alphaBlendingShader->setUniformMat4("view", _camera->getViewMatrix());
    _alphaBlendingShader->setUniformMat4("model", _knot->transform.getLocalMatrix());
    // 2 set light
    _alphaBlendingShader->setUniformVec3("directionalLight.direction", _light->transform.getFront());
    _alphaBlendingShader->setUniformFloat("directionalLight.intensity", _light->intensity);
    _alphaBlendingShader->setUniformVec3("directionalLight.color", _light->color);
    // 3 set material
    _alphaBlendingShader->setUniformVec3("material.albedo", _knotMaterial->albedo);
    _alphaBlendingShader->setUniformFloat("material.ka", _knotMaterial->ka);
    _alphaBlendingShader->setUniformVec3("material.kd", _knotMaterial->kd);
    _alphaBlendingShader->setUniformFloat("material.transparent", _knotMaterial->transparent);

    // TODO: use two render passes to achieve alpha blending
    // pass 1: Write the depth info to the zbuffer, while leave the color buffer unmodified.
    //         This pass will record the depth info of the object to avoid backward parts to
    //         be rendered.
    // write your code here
    // ------------------------------------------------------------------------
    // ...
    // ------------------------------------------------------------------------

    _knot->draw();
    
    // pass 2: Write the color buffer using the zbuffer info from pass 1 with blending, 
    //           while leaving the depth buffer unmodified.
    // write your code here
    // ------------------------------------------------------------------------
    // ...
    // ------------------------------------------------------------------------

    _knot->draw();
    // restore: don't forget to restore the OpenGL state before pass 1, which will avoid side effects
    //          to the object rendering afterwards.
    // write your code here
    // ------------------------------------------------------------------------
    // ...
    // ------------------------------------------------------------------------
}

void Transparency::renderWithDepthPeeling() {
    const glm::mat4 projection = _camera->getProjectionMatrix();
    const glm::mat4 view = _camera->getViewMatrix();

    // 1. initialize min depth buffer
    _colorBlendFbo->bind();

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);

    _depthPeelingInitShader->use();
    // 1.1 set transformation matrices
    _depthPeelingInitShader->setUniformMat4("projection", projection);
    _depthPeelingInitShader->setUniformMat4("view", view);
    _depthPeelingInitShader->setUniformMat4("model", _knot->transform.getLocalMatrix());
    // 1.2 set light
    _depthPeelingInitShader->setUniformVec3("directionalLight.direction", _light->transform.getFront());
    _depthPeelingInitShader->setUniformFloat("directionalLight.intensity", _light->intensity);
    _depthPeelingInitShader->setUniformVec3("directionalLight.color", _light->color);
    // 1.3 set material
    _depthPeelingInitShader->setUniformVec3("material.albedo", _knotMaterial->albedo);
    _depthPeelingInitShader->setUniformFloat("material.ka", _knotMaterial->ka);
    _depthPeelingInitShader->setUniformVec3("material.kd", _knotMaterial->kd);
    _depthPeelingInitShader->setUniformFloat("material.transparent", _knotMaterial->transparent);

    _knot->draw();

    // 2. TODO: depth peeling and blending
    // hint1: this stage can be divided into iterative 2 pass: peeling pass and blending pass
    // hint2: use _fbos as ping-pong framebuffer for peeling pass
    // hint3: use _depthPeelingShader for peeling pass
    // hint4: use _colorBlendFbo for blending pass
    // hint5: use _depthPeelingBlendShader for blend pass
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
    _depthPeelingFinalShader->use();
    // 3.1 set the window extent
    _depthPeelingFinalShader->setUniformInt("windowExtent.width", _windowWidth);
    _depthPeelingFinalShader->setUniformInt("windowExtent.height", _windowHeight);
    // 3.2 set the blend texture
    _colorBlendTexture->bind(0);
    // 3.3 set the background color
    _depthPeelingFinalShader->setUniformVec4("backgroundColor", _clearColor);

    _fullscreenQuad->draw();
}

void Transparency::renderUI() {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    const auto flags =
        ImGuiWindowFlags_AlwaysAutoResize |
        ImGuiWindowFlags_NoSavedSettings;

    if (!ImGui::Begin("Control Panel", nullptr, flags)) {
        ImGui::End();
    } else {
        ImGui::Text("Render Mode");
        ImGui::Separator();
        ImGui::RadioButton("Alpha Testing", (int*)&_renderMode, (int)(RenderMode::AlphaTesting));
        ImGui::RadioButton("Alpha Blending", (int*)&_renderMode, (int)(RenderMode::AlphaBlending));
        ImGui::RadioButton("Depth Peeling", (int*)&_renderMode, (int)(RenderMode::DepthPeeling));
        ImGui::SliderFloat("transparent", &_knotMaterial->transparent, 0.0f, 1.0f);
        ImGui::NewLine();

        ImGui::ColorEdit3("background", (float*)&_clearColor);

        ImGui::End();
    }

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}