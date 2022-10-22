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
    _bunny.reset(new Model(getAssetFullPath(bunnyRelPath)));
    _bunny->transform.position = glm::vec3(0.0f, 2.5f, 0.0f);

    _camera.reset(new PerspectiveCamera(
		glm::radians(60.0f), static_cast<float>(_windowWidth) / _windowHeight, 0.1f, 1000.0f));
    _camera->transform.position = glm::vec3(4.0f, 6.0f, 10.0f);
    _camera->transform.lookAt(glm::vec3(0.0f));

    _pointLight.reset(new PointLight);
    _pointLight->transform.position = glm::vec3(4.0f, 4.0f, 4.0f);
    _pointLight->transform.scale = glm::vec3(0.3f, 0.3f, 0.3f);
    _pointLight->color = glm::vec3(0.6f, 0.6f, 0.6f);
    _pointLight->intensity = 3.0f;
    _pointLight->kc = 1.0f;
    _pointLight->kl = 0.0f;
    _pointLight->kq = 0.05f;

    _sphere.reset(new Model(getAssetFullPath(sphereRelPath)));

    _cube.reset(new Model(getAssetFullPath(cubeRelPath)));
    _cube->transform.position = glm::vec3(22.5659f, 25.1945f, 0.0f);
    _cube->transform.scale = glm::vec3(50.0f);

    _screenQuad.reset(new FullscreenQuad);

    initGeometryPassResources();

    initSSAOPassResources();

    initBloomPassResources();

    initShaders();

    // init imGUI
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(_window, true);
    ImGui_ImplOpenGL3_Init();
}

PostProcessing::~PostProcessing() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

void PostProcessing::handleInput() {
    if (_input.keyboard.keyStates[GLFW_KEY_ESCAPE] != GLFW_RELEASE) {
        glfwSetWindowShouldClose(_window, true);
        return;
    }
}

void PostProcessing::renderFrame() {
    renderScene();
    renderUI();
}

void  PostProcessing::initGeometryPassResources() {
    _gPosition.reset(new Texture2D(GL_RGB32F, _windowWidth, _windowHeight, GL_RGB, GL_FLOAT));
    _gPosition->bind();
    _gPosition->setParamterInt(GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    _gPosition->setParamterInt(GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    _gPosition->setParamterInt(GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    _gPosition->setParamterInt(GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    _gNormal.reset(new Texture2D(GL_RGB32F, _windowWidth, _windowHeight, GL_RGB, GL_FLOAT));
    _gNormal->bind();
    _gNormal->setParamterInt(GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    _gNormal->setParamterInt(GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    _gNormal->setParamterInt(GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    _gNormal->setParamterInt(GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    _gAlbedo.reset(new Texture2D(GL_RGB32F, _windowWidth, _windowHeight, GL_RGB, GL_FLOAT));
    _gAlbedo->bind();
    _gAlbedo->setParamterInt(GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    _gAlbedo->setParamterInt(GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    _gAlbedo->setParamterInt(GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    _gAlbedo->setParamterInt(GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    _gBufferFBO->attachTexture(*_gAlbedo, GL_COLOR_ATTACHMENT2);

    _gDepth.reset(new Texture2D(GL_DEPTH_COMPONENT, _windowWidth, _windowHeight, GL_DEPTH_COMPONENT, GL_FLOAT));
    _gDepth->bind();
    _gDepth->setParamterInt(GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    _gDepth->setParamterInt(GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    _gDepth->setParamterInt(GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    _gDepth->setParamterInt(GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    _gBufferFBO.reset(new Framebuffer);
    _gBufferFBO->bind();
    _gBufferFBO->drawBuffers({
        GL_COLOR_ATTACHMENT0,
        GL_COLOR_ATTACHMENT1,
        GL_COLOR_ATTACHMENT2 });
    _gBufferFBO->attachTexture(*_gPosition, GL_COLOR_ATTACHMENT0);
    _gBufferFBO->attachTexture(*_gNormal, GL_COLOR_ATTACHMENT1);
    _gBufferFBO->attachTexture(*_gAlbedo, GL_COLOR_ATTACHMENT2);
    _gBufferFBO->attachTexture(*_gDepth, GL_DEPTH_ATTACHMENT);
    _gBufferFBO->unbind();

    _gBufferShader.reset(new GLSLProgram);
    _gBufferShader->attachVertexShaderFromFile(getAssetFullPath(geometryVsRelPath));
    _gBufferShader->attachFragmentShaderFromFile(getAssetFullPath(geometryFsRelPath));
    _gBufferShader->link();
}

void PostProcessing::initSSAOPassResources() {
    _ssaoFBO.reset(new Framebuffer);
    _ssaoFBO->bind();
    _ssaoFBO->drawBuffer(GL_COLOR_ATTACHMENT0);
    for (int i = 0; i < 2; ++i) {
        _ssaoResult[i].reset(new Texture2D(GL_R32F, _windowWidth, _windowHeight, GL_RED, GL_FLOAT));
        _ssaoResult[i]->bind();
        _ssaoResult[i]->setParamterInt(GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        _ssaoResult[i]->setParamterInt(GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        _ssaoResult[i]->setParamterInt(GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        _ssaoResult[i]->setParamterInt(GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    }
    _ssaoFBO->attachTexture(*_ssaoResult[0], GL_COLOR_ATTACHMENT0);
    _ssaoFBO->unbind();

    _ssaoBlurFBO.reset(new Framebuffer);
    _ssaoBlurFBO->bind();
    _ssaoBlurFBO->drawBuffer(GL_COLOR_ATTACHMENT0);
    _ssaoBlurFBO->unbind();

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
        _sampleVecs.push_back(sample);
    }

    std::vector<glm::vec3> ssaoNoises;
    for (unsigned int i = 0; i < 16; i++) {
        // rotate around z-axis (in tangent space)
        glm::vec3 noise(u(e) * 2.0f - 1.0f, u(e) * 2.0f - 1.0f, 0.0f);
        ssaoNoises.push_back(noise);
    }

    _ssaoNoise.reset(new Texture2D(GL_RGB32F, 4, 4, GL_RGB, GL_FLOAT, ssaoNoises.data()));
    _ssaoNoise->bind();
    _ssaoNoise->setParamterInt(GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    _ssaoNoise->setParamterInt(GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    _ssaoNoise->setParamterInt(GL_TEXTURE_WRAP_S, GL_REPEAT);
    _ssaoNoise->setParamterInt(GL_TEXTURE_WRAP_T, GL_REPEAT);
    _ssaoNoise->unbind();

    // TODO: modify ssao.frag
    _ssaoShader.reset(new GLSLProgram);
    _ssaoShader->attachVertexShaderFromFile(getAssetFullPath(quadVsRelPath));
    _ssaoShader->attachFragmentShaderFromFile(getAssetFullPath(ssaoFsRelPath));
    _ssaoShader->link();

    _ssaoBlurShader.reset(new GLSLProgram);
    _ssaoBlurShader->attachVertexShaderFromFile(getAssetFullPath(quadVsRelPath));
    _ssaoBlurShader->attachFragmentShaderFromFile(getAssetFullPath(ssaoBlurFsRelPath));
    _ssaoBlurShader->link();

    // TODO: modify ssao_lighting.frag
    _ssaoLightingShader.reset(new GLSLProgram);
    _ssaoLightingShader->attachVertexShaderFromFile(getAssetFullPath(quadVsRelPath));
    _ssaoLightingShader->attachFragmentShaderFromFile(getAssetFullPath(ssaoLightingFsRelPath));
    _ssaoLightingShader->link();
}

void PostProcessing::initBloomPassResources() {
    _bloomFBO.reset(new Framebuffer);
    _bloomFBO->bind();
    _bloomFBO->drawBuffer(GL_COLOR_ATTACHMENT0);

    _bloomMap.reset(new Texture2D(GL_RGBA32F, _windowWidth, _windowHeight, GL_RGBA, GL_FLOAT));
    _bloomMap->bind();
    _bloomMap->setParamterInt(GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    _bloomMap->setParamterInt(GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    _bloomMap->setParamterInt(GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    _bloomMap->setParamterInt(GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    _bloomFBO->attachTexture(*_bloomMap, GL_COLOR_ATTACHMENT0);
    for (int i = 0; i < 2; ++i) {
        _brightColorMap[i].reset(new Texture2D(GL_RGBA32F, _windowWidth, _windowHeight, GL_RGBA, GL_FLOAT));
        _brightColorMap[i]->bind();
        _brightColorMap[i]->setParamterInt(GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        _brightColorMap[i]->setParamterInt(GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        _brightColorMap[i]->setParamterInt(GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        _brightColorMap[i]->setParamterInt(GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    }

    _bloomFBO->attachTexture(*_gDepth, GL_DEPTH_ATTACHMENT);
    _bloomFBO->unbind();

    _brightColorFBO.reset(new Framebuffer);
    _brightColorFBO->bind();
    _brightColorFBO->drawBuffer(GL_COLOR_ATTACHMENT0);
    _brightColorFBO->unbind();

    _blurFBO.reset(new Framebuffer);
    _blurFBO->bind();
    _blurFBO->drawBuffer(GL_COLOR_ATTACHMENT0);
    _blurFBO->unbind();

    _lightShader.reset(new GLSLProgram);
    _lightShader->attachVertexShaderFromFile(getAssetFullPath(lightVsRelPath));
    _lightShader->attachFragmentShaderFromFile(getAssetFullPath(lightFsRelPath));
    _lightShader->link();

    // TODO: modify extract_bright_color.frag
    _brightColorShader.reset(new GLSLProgram);
    _brightColorShader->attachVertexShaderFromFile(getAssetFullPath(quadVsRelPath));
    _brightColorShader->attachFragmentShaderFromFile(getAssetFullPath(brightColorFsRelPath));
    _brightColorShader->link();

    // TODO: modify gaussian_blur.frag
    _blurShader.reset(new GLSLProgram);
    _blurShader->attachVertexShaderFromFile(getAssetFullPath(quadVsRelPath));
    _blurShader->attachFragmentShaderFromFile(getAssetFullPath(gaussianBlurFsRelPath));
    _blurShader->link();

    _blendShader.reset(new GLSLProgram);
    _blendShader->attachVertexShaderFromFile(getAssetFullPath(quadVsRelPath));
    _blendShader->attachFragmentShaderFromFile(getAssetFullPath(blendBloomMapFsRelPath));
    _blendShader->link();
}

void PostProcessing::initShaders() {
    _drawScreenShader.reset(new GLSLProgram);
    _drawScreenShader->attachVertexShaderFromFile(getAssetFullPath(quadVsRelPath));
    _drawScreenShader->attachFragmentShaderFromFile(getAssetFullPath(quadFsRelPath));
    _drawScreenShader->link();
}

void PostProcessing::renderScene() {
    glClearColor(_clearColor.r, _clearColor.g, _clearColor.b, _clearColor.a);
    // deferred rendering: geometry pass
    _gBufferFBO->bind();
    glEnable(GL_DEPTH_TEST);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    _gBufferShader->use();
    _gBufferShader->setUniformMat4("projection", _camera->getProjectionMatrix());
    _gBufferShader->setUniformMat4("view", _camera->getViewMatrix());

    _gBufferShader->setUniformMat4("model", _bunny->transform.getLocalMatrix());
    _bunny->draw();

    _gBufferShader->setUniformMat4("model", _cube->transform.getLocalMatrix());
    _cube->draw();

    _gBufferFBO->unbind();

    // deferred rendering: lighting passes
    // + SSAO pass
    if (_enableSSAO) {
        _ssaoFBO->bind();
        _ssaoFBO->attachTexture(*_ssaoResult[0], GL_COLOR_ATTACHMENT0);

        glDisable(GL_DEPTH_TEST);

        _ssaoShader->use();
        _ssaoShader->setUniformInt("gPosition", 0);
        _gPosition->bind(0);
        _ssaoShader->setUniformInt("gNormal", 1);
        _gNormal->bind(1);
        _ssaoShader->setUniformInt("noiseMap", 2);
        _ssaoNoise->bind(2);
        for (int i = 0; i < _sampleVecs.size(); ++i) {
            _ssaoShader->setUniformVec3("sampleVecs[" + std::to_string(i) + "]", _sampleVecs[i]);
        }

        _ssaoShader->setUniformInt("screenWidth", _windowWidth);
        _ssaoShader->setUniformInt("screenHeight", _windowHeight);
        _ssaoShader->setUniformMat4("projection", _camera->getProjectionMatrix());
        _screenQuad->draw();

        _ssaoFBO->unbind();
        _ssaoBlurFBO->bind();
        glDisable(GL_DEPTH_TEST);
        _currentReadBuffer = 0;
        _currentWriteBuffer = 1;
        _ssaoBlurShader->use();
        for (int pass = 0; pass < 5; ++pass) {
            _ssaoBlurFBO->attachTexture(*_ssaoResult[_currentWriteBuffer], GL_COLOR_ATTACHMENT0);
            _ssaoBlurShader->setUniformInt("ssaoResult", 0);
            _ssaoResult[_currentReadBuffer]->bind(0);
            _screenQuad->draw();
            std::swap(_currentReadBuffer, _currentWriteBuffer);
        }
        _ssaoBlurFBO->unbind();
    } else {
        _currentReadBuffer = 0;
        static const std::vector<float> ones(_windowWidth * _windowHeight, 1.0f);
        _ssaoResult[0]->bind();
        glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, _windowWidth, _windowHeight, 0, GL_RED, GL_FLOAT, ones.data());
        _ssaoResult[0]->unbind();
    }

    // + bloom pass
    _bloomFBO->bind();
    glDisable(GL_DEPTH_TEST);
    glClear(GL_COLOR_BUFFER_BIT);

    _ssaoLightingShader->use();

    _ssaoLightingShader->setUniformVec3("light.position", _pointLight->transform.position);
    _ssaoLightingShader->setUniformVec3("light.color", _pointLight->color);
    _ssaoLightingShader->setUniformFloat("light.intensity", _pointLight->intensity);
    _ssaoLightingShader->setUniformFloat("light.kc", _pointLight->kc);
    _ssaoLightingShader->setUniformFloat("light.kl", _pointLight->kl);
    _ssaoLightingShader->setUniformFloat("light.kq", _pointLight->kq);
    
    _ssaoLightingShader->setUniformInt("gPosition", 0);
    _gPosition->bind(0);
    _ssaoLightingShader->setUniformInt("gNormal", 1);
    _gNormal->bind(1);
    _ssaoLightingShader->setUniformInt("gAlbedo", 2);
    _gAlbedo->bind(2);
    _ssaoLightingShader->setUniformInt("ssaoResult", 3);
    _ssaoResult[_currentReadBuffer]->bind(3);

    _screenQuad->draw();

    glEnable(GL_DEPTH_TEST);
    _lightShader->use();

    _lightShader->setUniformMat4("projection", _camera->getProjectionMatrix());
    _lightShader->setUniformMat4("view", _camera->getViewMatrix());
    _lightShader->setUniformMat4("model", _pointLight->transform.getLocalMatrix());
    _lightShader->setUniformVec3("lightColor", _pointLight->color);
    _lightShader->setUniformFloat("lightIntensity", _pointLight->intensity);
    
    _sphere->draw();

    _bloomFBO->unbind();

    if (_enableBloom) {
        extractBrightColor(*_bloomMap);
        blurBrightColor();
        combineSceneMapAndBloomBlur(*_bloomMap);
    } else {
        glDisable(GL_DEPTH_TEST);
        _drawScreenShader->use();
        _drawScreenShader->setUniformInt("frame", 0);
        _bloomMap->bind(0);
        _screenQuad->draw();
    }
}

void PostProcessing::renderUI() {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    const auto flags =
        ImGuiWindowFlags_AlwaysAutoResize |
        ImGuiWindowFlags_NoSavedSettings;

    if (!ImGui::Begin("Control Panel", nullptr, flags)) {
        ImGui::End();
    } else {
        ImGui::Text("post processing technics");
        ImGui::Separator();
        ImGui::Checkbox("bloom", &_enableBloom);
        ImGui::Checkbox("ssao", &_enableSSAO);

        ImGui::End();
    }

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void PostProcessing::extractBrightColor(const Texture2D& sceneMap) {
    _brightColorFBO->bind();
    _brightColorFBO->attachTexture(*_brightColorMap[0], GL_COLOR_ATTACHMENT0);
    _brightColorShader->use();
    _brightColorShader->setUniformInt("sceneMap", 0);
    sceneMap.bind(0);
    _screenQuad->draw();
    _brightColorFBO->unbind();
}

void PostProcessing::blurBrightColor() {
    _blurFBO->bind();
    _blurFBO->drawBuffer(GL_COLOR_ATTACHMENT0);
    _blurShader->use();
    bool horizontal = true;
    _blurShader->setUniformInt("image", 0);
    _currentReadBuffer = 0;
    _currentWriteBuffer = 1;

    for (int pass = 0; pass < 20; ++pass) {
        _blurShader->setUniformBool("horizontal", horizontal);
        _blurFBO->attachTexture(*_brightColorMap[_currentWriteBuffer], GL_COLOR_ATTACHMENT0);
        _brightColorMap[_currentReadBuffer]->bind(0);
        _screenQuad->draw();
        horizontal = !horizontal;
        std::swap(_currentReadBuffer, _currentWriteBuffer);
    }

    _blurFBO->unbind();
}

void PostProcessing::combineSceneMapAndBloomBlur(const Texture2D& sceneMap) {
    glDisable(GL_DEPTH_TEST);
    _blendShader->use();

    _blendShader->setUniformInt("scene", 0);
    sceneMap.bind(0);

    _blendShader->setUniformInt("bloomBlur", 1);
    _brightColorMap[_currentReadBuffer]->bind(1);

    _screenQuad->draw();
}