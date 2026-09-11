#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include "debug_print.h"
#include "pbr_viewer.h"

const std::string modelRelPath = "gltf/DamagedHelmet.gltf";
// const std::string modelRelPath = "gltf/drone/scene.gltf";
// const std::string modelRelPath = "gltf/grey_knight/scene.gltf";

const std::string pbrVertShaderRelPath = "shader/pbr_viewer/pbr.vert";
const std::string pbrFragShaderRelPath = "shader/pbr_viewer/pbr.frag";

const std::string skyboxVertShaderRelPath = "shader/pbr_viewer/skybox.vert";
const std::string skyboxFragShaderRelPath = "shader/pbr_viewer/skybox.frag";

const std::string equirectVertShaderRelPath = "shader/pbr_viewer/filter_cube.vert";
const std::string equirectFragShaderRelPath = "shader/pbr_viewer/equirectangular_to_cubemap.frag";

const std::string irradianceVertShaderRelPath = "shader/pbr_viewer/filter_cube.vert";
const std::string irradianceFragShaderRelPath = "shader/pbr_viewer/irradiance.frag";

const std::string prefilterVertShaderRelPath = "shader/pbr_viewer/filter_cube.vert";
const std::string prefilterFragShaderRelPath = "shader/pbr_viewer/prefilter.frag";

const std::string brdfLutVertShaderRelPath = "shader/pbr_viewer/brdf_lut.vert";
const std::string brdfLutFragShaderRelPath = "shader/pbr_viewer/brdf_lut.frag";

const std::string quadVertShaderRelPath = "shader/pbr_viewer/quad.vert";
const std::string quadFragShaderRelPath = "shader/pbr_viewer/quad.frag";

const std::string skyboxTextureRelPaths = "texture/hdr/newport_loft.hdr";

PbrViewer::PbrViewer(const Options& options) : Application(options) {
    m_model.reset(new Model(getAssetFullPath(modelRelPath)));

    // camera
    m_camera.reset(new PerspectiveCamera(
        glm::radians(45.0f), 1.0f * m_windowWidth / m_windowHeight, 0.1f, 10000.0f));
    m_camera->transform.position = {0.0f, 0.0f, 5.0f};
    // camera controller
    m_cameraController.reset(
        new CameraController(*m_camera, glm::vec3(0.0f), m_windowWidth, m_windowHeight));

    // lights
    m_directionalLight.reset(new DirectionalLight());
    m_directionalLight->transform.rotation = glm::quatLookAt(
        {sin(glm::radians(75.0f)) * cos(glm::radians(45.0f)), sin(glm::radians(45.0f)),
         cos(glm::radians(75.0f)) * cos(glm::radians(45.0f))},
        Transform::getDefaultUp());
    m_directionalLight->intensity = 10.0f;

    // skybox
    glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

    m_skybox.reset(new Skybox(
        getAssetFullPath(skyboxTextureRelPaths), getAssetFullPath(equirectVertShaderRelPath),
        getAssetFullPath(equirectFragShaderRelPath), 512));

    m_skybox->generateIrradianceMap(
        getAssetFullPath(irradianceVertShaderRelPath),
        getAssetFullPath(irradianceFragShaderRelPath), 32, glm::radians(1.0f), glm::radians(1.0f));

    m_skybox->generatePrefilterMap(
        getAssetFullPath(prefilterVertShaderRelPath), getAssetFullPath(prefilterFragShaderRelPath),
        128, 4096);

    m_skybox->generateBrdfLutMap(
        getAssetFullPath(brdfLutVertShaderRelPath), getAssetFullPath(brdfLutFragShaderRelPath), 512,
        4096);

    // fullscreen quad
    m_quad.reset(new FullscreenQuad);

    initShaders();

    setupUniformBufferObjects();

    confirmBindingPoints();

    // init imGUI
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    (void)io;

    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(m_window, true);
    ImGui_ImplOpenGL3_Init();
}

PbrViewer::~PbrViewer() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

void PbrViewer::handleInput() {
    if (m_input.keyboard.keyStates[GLFW_KEY_ESCAPE] != GLFW_RELEASE) {
        glfwSetWindowShouldClose(m_window, true);
        return;
    }

    if (!ImGui::GetIO().WantCaptureMouse) {
        m_cameraController->update(m_input, m_deltaTime);
    }

    m_input.forwardState();

    updateUniforms();
    enqueueRenderables();

    // printRenderQueue("Opaque Queue", m_opaqueQueue);
    // printRenderQueue("Alpha Queue", m_alphaQueue);
    // printRenderQueue("Transparent Queue", m_transparentQueue);
}

void PbrViewer::enqueueRenderables() {
    m_opaqueQueue.clear();
    m_alphaQueue.clear();
    m_transparentQueue.clear();

    static glm::mat4 globalMatrix = glm::mat4(1.0f);
    // globalMatrix = glm::rotate(globalMatrix, m_deltaTime * 1.0f, glm::vec3(0.0f, 1.0f, 0.0f));
    for (const Node* node : m_model->getRootNodes()) {
        enqueueRenderable(*node, globalMatrix);
    }
}

void PbrViewer::enqueueRenderable(const Node& node, glm::mat4 parentGlobalMatrix) {
    glm::mat4 nodeGlobalMatrix = parentGlobalMatrix * node.transform.getLocalMatrix();

    for (const auto& primitive : node.primitives) {
        RenderObject object = {nodeGlobalMatrix, &primitive};
        switch (primitive.material->alphaMode) {
        case Material::AlphaMode::Opaque: m_opaqueQueue.emplace_back(object); break;
        case Material::AlphaMode::Mask: m_alphaQueue.emplace_back(object); break;
        case Material::AlphaMode::Blend: m_transparentQueue.emplace_back(object); break;
        }
    }

    for (const Node* childNode : node.children) {
        enqueueRenderable(*childNode, nodeGlobalMatrix);
    }
}

void PbrViewer::drawPrimitive(const Primitive& primitive) const {
    // double sided
    bool doubleSided = primitive.material->doubleSided;
    if (!doubleSided) {
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);
    }

    // draw call
    glBindVertexArray(primitive.vertexArray);
    if (primitive.indexCount > 0) {
        glDrawElements(
            GL_TRIANGLES, primitive.indexCount, GL_UNSIGNED_INT,
            (GLvoid*)(sizeof(uint32_t) * primitive.firstIndex));
    } else {
        glDrawArrays(GL_TRIANGLES, primitive.firstVertex, primitive.vertexCount);
    }
    glBindVertexArray(0);

    // restore opengl state
    if (!doubleSided) {
        glDisable(GL_CULL_FACE);
    }
}

void PbrViewer::renderFrame() {
    showFpsInWindowTitle();
    clearScreen();

    renderOpaqueQueue();

    renderAlphaQueue();

    renderSkybox();

    renderTransparentQueue();

    renderUI();
}

void PbrViewer::clearScreen() {
    glClearColor(m_clearColor.r, m_clearColor.g, m_clearColor.b, m_clearColor.a);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
}

void PbrViewer::updateUniforms() {
    // camera
    m_uboCamera->update("projection", m_camera->getProjectionMatrix());
    m_uboCamera->update("view", m_camera->getViewMatrix());
    m_uboCamera->update("viewPosition", m_camera->transform.position);

    // lights
    m_uboLights->update("directionalLightCount", 1);
    m_uboLights->update("directionalLights[0].direction", m_directionalLight->transform.getFront());
    m_uboLights->update("directionalLights[0].color", m_directionalLight->color);
    m_uboLights->update("directionalLights[0].intensity", m_directionalLight->intensity);
    m_uboLights->update("pointLightCount", 0);
    m_uboLights->update("spotLightCount", 0);

    // IBL
    m_uboEnvironment->update("exposure", m_skybox->exposure);
    m_uboEnvironment->update("gamma", m_skybox->gamma);
    m_uboEnvironment->update("maxPrefilterMipLevel", m_skybox->getMaxPrefilterMipLevel());
    m_uboEnvironment->update("scaleIBLAmbient", m_skybox->scaleIBLAmbient);
}

void PbrViewer::renderOpaqueQueue() const {
    for (const auto& object : m_opaqueQueue) {
        m_pbrShader->use();
        setPbrShaderUniforms(object);
        drawPrimitive(*object.primitive);
    }
}

void PbrViewer::renderAlphaQueue() const {
    for (const auto& object : m_alphaQueue) {
        m_pbrShader->use();
        setPbrShaderUniforms(object);
        drawPrimitive(*object.primitive);
    }
}

void PbrViewer::renderTransparentQueue() const {
    // TODO: sort the object from near to far
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    for (const auto& object : m_transparentQueue) {
        m_pbrShader->use();
        setPbrShaderUniforms(object);
        drawPrimitive(*object.primitive);
    }
    glDisable(GL_BLEND);
}

void PbrViewer::renderSkybox() const {
    switch (m_skyboxRenderMode) {
    case SkyboxRenderMode::Irradiance: renderIrradianceMap(); break;
    case SkyboxRenderMode::Prefilter: renderPrefilterMap(); break;
    case SkyboxRenderMode::BrdfLut: renderBrdfLutMap(); break;
    default:
        glDepthFunc(GL_LEQUAL);
        m_skyboxShader->use();
        m_skyboxShader->setUniformInt("environmentMap", 0);
        m_skyboxShader->setUniformFloat("lod", m_skybox->backgroundLod);
        m_skybox->bindEnvironmentMap(0);
        glBindSampler(0, 0);
        m_skybox->draw();
        glDepthFunc(GL_LESS);
        break;
    }
}

void PbrViewer::renderIrradianceMap() const {
    glDepthFunc(GL_LEQUAL);
    m_skyboxShader->use();
    m_skyboxShader->setUniformInt("environmentMap", 0);
    m_skyboxShader->setUniformFloat("lod", 0.0f);
    m_skybox->irradianceMap->bind(0);
    glBindSampler(0, 0);
    m_skybox->draw();
    glDepthFunc(GL_LESS);
}

void PbrViewer::renderPrefilterMap() const {
    glDepthFunc(GL_LEQUAL);
    m_skyboxShader->use();
    m_skyboxShader->setUniformInt("environmentMap", 0);
    m_skyboxShader->setUniformFloat("environmentMap", m_skybox->backgroundLod);
    m_skybox->prefilterMap->bind(0);
    glBindSampler(0, 0);
    m_skybox->draw();
    glDepthFunc(GL_LESS);
}

void PbrViewer::renderBrdfLutMap() const {
    m_quadShader->use();
    m_quadShader->setUniformInt("inputTexture", 0);
    m_skybox->brdfLutMap->bind(0);
    glBindSampler(0, 0);
    m_quad->draw();
}

void PbrViewer::renderUI() const {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    const auto flags = ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings;

    if (!ImGui::Begin("Control Panel", nullptr, flags)) {
        ImGui::End();
    } else {
        if (ImGui::CollapsingHeader("Lighting", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::Text("directional light");
            ImGui::Separator();
            ImGui::SliderFloat("intensity", &m_directionalLight->intensity, 0.0f, 20.0f);
            ImGui::ColorEdit3("color", (float*)&m_directionalLight->color);
            ImGui::Text("image based lighting");
            ImGui::Separator();
            ImGui::SliderFloat("blur", &m_skybox->backgroundLod, 0.0f, 8.0f);
            // ImGui::SliderFloat("exposure", &m_skybox->exposure, 0.0f, 10.0f);
            // ImGui::SliderFloat("gamma", &m_skybox->gamma, 0.1f, 4.0f);
            ImGui::SliderFloat("scale", &m_skybox->scaleIBLAmbient, 0.0f, 1.5f);
        }

        if (ImGui::CollapsingHeader("Debug View", ImGuiTreeNodeFlags_DefaultOpen)) {
            static const char* pbrChannels[] = {"All",    "Albedo",    "Roughness", "Metallic",
                                                "Normal", "Occlusion", "Emissive"};

            ImGui::Combo(
                "pbr channel", (int*)(&m_debugInput), pbrChannels, IM_ARRAYSIZE(pbrChannels));

            static const char* skyboxTextureItems[] = {"Raw", "Irradiance", "Prefilter", "BrdfLut"};

            ImGui::Combo(
                "skybox texture", (int*)(&m_skyboxRenderMode), skyboxTextureItems,
                IM_ARRAYSIZE(skyboxTextureItems));
        }

        ImGui::End();
    }

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void PbrViewer::initShaders() {
    m_pbrShader.reset(new GLSLProgram);
    m_pbrShader->attachVertexShaderFromFile(getAssetFullPath(pbrVertShaderRelPath));
    m_pbrShader->attachFragmentShaderFromFile(getAssetFullPath(pbrFragShaderRelPath));
    m_pbrShader->link();

    m_skyboxShader.reset(new GLSLProgram);
    m_skyboxShader->attachVertexShaderFromFile(getAssetFullPath(skyboxVertShaderRelPath));
    m_skyboxShader->attachFragmentShaderFromFile(getAssetFullPath(skyboxFragShaderRelPath));
    m_skyboxShader->link();

    m_quadShader.reset(new GLSLProgram);
    m_quadShader->attachVertexShaderFromFile(getAssetFullPath(quadVertShaderRelPath));
    m_quadShader->attachFragmentShaderFromFile(getAssetFullPath(quadFragShaderRelPath));
    m_quadShader->link();
}

void PbrViewer::setPbrShaderUniforms(const RenderObject& object) const {
    const PbrMaterial* material = object.primitive->material;
    m_pbrShader->setUniformMat4("model", object.globalMatrix);
    m_pbrShader->setUniformVec4("material.albedoFactor", material->albedoFactor);
    m_pbrShader->setUniformVec4("material.emissiveFactor", material->emissiveFactor);
    m_pbrShader->setUniformFloat("material.metallicFactor", material->metallicFactor);
    m_pbrShader->setUniformFloat("material.roughnessFactor", material->roughnessFactor);
    m_pbrShader->setUniformFloat("material.occlusionStrength", material->occlusionStrength);
    m_pbrShader->setUniformInt("material.albedoTexCoordSet", material->texCoordSets.albedo);
    m_pbrShader->setUniformInt("material.metallicTexCoordSet", material->texCoordSets.metallic);
    m_pbrShader->setUniformInt("material.roughnessTexCoordSet", material->texCoordSets.roughness);
    m_pbrShader->setUniformInt("material.normalTexCoordSet", material->texCoordSets.normal);
    m_pbrShader->setUniformInt("material.emissiveTexCoordSet", material->texCoordSets.emissive);
    m_pbrShader->setUniformInt("material.occlusionTexCoordSet", material->texCoordSets.occlusion);

    m_pbrShader->setUniformBool("material.doubleSided", material->doubleSided);

    switch (material->alphaMode) {
    case Material::AlphaMode::Opaque:
        m_pbrShader->setUniformBool("material.alphaMask", false);
        break;
    case Material::AlphaMode::Mask: m_pbrShader->setUniformBool("material.alphaMask", true); break;
    case Material::AlphaMode::Blend:
        // try to discard to opacity that is near zero:
        // when occur the blend matrial, the data parser has set alphaMask false,
        // but the alphaCutoff 0.05, which should have been ignored when alpha mode is blend
        // we can use the thick to discard the unwanted opacity texture
        m_pbrShader->setUniformBool("material.alphaMask", true);
        break;
    }
    m_pbrShader->setUniformFloat("material.alphaMaskCutoff", material->alphaCutoff);

    // textures
    if (material->albedoMap && material->texCoordSets.albedo >= 0) {
        m_pbrShader->setUniformInt("albedoMap", 0);
        material->albedoMap->bind(0);
        if (material->albeodoSampler) {
            material->albeodoSampler->bind(0);
        }
    }

    if (material->roughnessMap && material->texCoordSets.roughness >= 0) {
        m_pbrShader->setUniformInt("roughnessMap", 1);
        material->roughnessMap->bind(1);
        if (material->roughnessSampler) {
            material->roughnessSampler->bind(1);
        }
    }

    if (material->metallicMap && material->texCoordSets.metallic >= 0) {
        m_pbrShader->setUniformInt("metallicMap", 2);
        material->metallicMap->bind(2);
        if (material->metallicSampler) {
            material->metallicSampler->bind(2);
        }
    }

    if (material->normalMap && material->texCoordSets.normal >= 0) {
        m_pbrShader->setUniformInt("normalMap", 3);
        material->normalMap->bind(3);
        if (material->normalSampler) {
            material->normalSampler->bind(3);
        }
    }

    if (material->occlusionMap && material->texCoordSets.occlusion >= 0) {
        m_pbrShader->setUniformInt("occlusionMap", 4);
        material->occlusionMap->bind(4);
        if (material->occlusionSampler) {
            material->occlusionSampler->bind(4);
        }
    }

    if (material->emissiveMap && material->texCoordSets.emissive >= 0) {
        m_pbrShader->setUniformInt("emissiveMap", 5);
        material->emissiveMap->bind(5);
        if (material->emissiveSampler) {
            material->emissiveSampler->bind(5);
        }
    }

    // IBL textures
    m_pbrShader->setUniformInt("irradianceMap", 6);
    m_skybox->irradianceMap->bind(6);

    m_pbrShader->setUniformInt("prefilterMap", 7);
    m_skybox->prefilterMap->bind(7);

    m_pbrShader->setUniformInt("brdfLutMap", 8);
    m_skybox->brdfLutMap->bind(8);

    // debug
    m_pbrShader->setUniformInt("debugInput", static_cast<int>(m_debugInput));
}

void PbrViewer::setupUniformBufferObjects() {
    // uboCamera
    int uboCameraSize = m_pbrShader->getUniformBlockSize("uboCamera");
    if (uboCameraSize <= 0) {
        throw std::runtime_error("get uboCamera size failure");
    }

    m_uboCamera.reset(new UniformBuffer(uboCameraSize, GL_DYNAMIC_DRAW));

    std::string uboCameraVariableNames[] = {
        "projection",
        "view",
        "viewPosition",
    };

    for (const auto& name : uboCameraVariableNames) {
        int offset = m_pbrShader->getUniformBlockVariableOffset(name);
        if (offset <= -1) {
            throw std::runtime_error("get uboCamera." + name + " offset failure");
        } else {
            m_uboCamera->setOffset(name, static_cast<size_t>(offset));
        }
    }

    // uboLights
    int uboLightsSize = m_pbrShader->getUniformBlockSize("uboLights");
    if (uboLightsSize <= 0) {
        throw std::runtime_error("get uboLights size failure");
    }

    m_uboLights.reset(new UniformBuffer(uboLightsSize, GL_DYNAMIC_DRAW));

    std::vector<std::string> uboLightsVariableNames = {
        "directionalLightCount", "pointLightCount", "spotLightCount"};

    constexpr int maxDirectionalLights = 4;
    for (int i = 0; i < maxDirectionalLights; ++i) {
        std::string prefix = "directionalLights[" + std::to_string(i) + "]";
        uboLightsVariableNames.push_back(prefix + ".direction");
        uboLightsVariableNames.push_back(prefix + ".color");
        uboLightsVariableNames.push_back(prefix + ".intensity");
    }

    constexpr int maxPointLights = 8;
    for (int i = 0; i < maxPointLights; ++i) {
        std::string prefix = "pointLights[" + std::to_string(i) + "]";
        uboLightsVariableNames.push_back(prefix + ".position");
        uboLightsVariableNames.push_back(prefix + ".direction");
        uboLightsVariableNames.push_back(prefix + ".color");
        uboLightsVariableNames.push_back(prefix + ".intensity");
        uboLightsVariableNames.push_back(prefix + ".kc");
        uboLightsVariableNames.push_back(prefix + ".kl");
        uboLightsVariableNames.push_back(prefix + ".kq");
    }

    constexpr int maxSpotLights = 8;
    for (int i = 0; i < maxSpotLights; ++i) {
        std::string prefix = "spotLights[" + std::to_string(i) + "]";
        uboLightsVariableNames.push_back(prefix + ".position");
        uboLightsVariableNames.push_back(prefix + ".direction");
        uboLightsVariableNames.push_back(prefix + ".color");
        uboLightsVariableNames.push_back(prefix + ".intensity");
        uboLightsVariableNames.push_back(prefix + ".kc");
        uboLightsVariableNames.push_back(prefix + ".kl");
        uboLightsVariableNames.push_back(prefix + ".kq");
        uboLightsVariableNames.push_back(prefix + ".angle");
    }

    for (const auto& name : uboLightsVariableNames) {
        int offset = m_pbrShader->getUniformBlockVariableOffset(name);
        if (offset <= -1) {
            throw std::runtime_error("get uboLights." + name + " offset failure");
        } else {
            m_uboLights->setOffset(name, static_cast<size_t>(offset));
        }
    }

    // uboEnvironment
    int uboEnvironmentSize = m_pbrShader->getUniformBlockSize("uboEnvironment");
    if (uboEnvironmentSize <= 0) {
        throw std::runtime_error("get uboEnvironment size failure");
    }
    m_uboEnvironment.reset(new UniformBuffer(uboEnvironmentSize, GL_DYNAMIC_DRAW));

    std::string uboEnvironmentNames[] = {
        "exposure", "gamma", "maxPrefilterMipLevel", "scaleIBLAmbient"};

    for (const auto& name : uboEnvironmentNames) {
        int offset = m_pbrShader->getUniformBlockVariableOffset(name);
        if (offset <= -1) {
            throw std::runtime_error("get uboEnvironment." + name + " offset failure");
        } else {
            m_uboEnvironment->setOffset(name, static_cast<size_t>(offset));
        }
    }
}

void PbrViewer::confirmBindingPoints() {
    // ubo binding point
    m_uboCamera->setBindingPoint(0);
    m_uboLights->setBindingPoint(1);
    m_uboEnvironment->setBindingPoint(2);

    // pbr shader binding point
    m_pbrShader->setUniformBlockBinding("uboCamera", 0);
    m_pbrShader->setUniformBlockBinding("uboLights", 1);
    m_pbrShader->setUniformBlockBinding("uboEnvironment", 2);

    // skybox shader binding point
    m_skyboxShader->setUniformBlockBinding("uboCamera", 0);
}

void PbrViewer::printRenderQueue(
    const std::string& name, const std::vector<RenderObject>& renderQueue) const {

    std::cout << "+ " << name << "\n";
    for (size_t i = 0; i < renderQueue.size(); ++i) {
        const glm::mat4 globalMatrix = glm::transpose(renderQueue[i].globalMatrix);
        const Primitive* primitive = renderQueue[i].primitive;
        std::cout << "  + object[" << i << "]:" << "\n";
        std::cout << "    + globalMatrix(row): " << "\n";
        std::cout << "        " << globalMatrix[0] << "\n";
        std::cout << "        " << globalMatrix[1] << "\n";
        std::cout << "        " << globalMatrix[2] << "\n";
        std::cout << "        " << globalMatrix[3] << "\n";
        std::cout << "    + vertexArray: " << primitive->vertexArray << "\n";
        std::cout << "    + firstVertex: " << primitive->firstVertex << "\n";
        std::cout << "    + VertexCount: " << primitive->vertexCount << "\n";
        std::cout << "    + firstIndex:  " << primitive->firstIndex << "\n";
        std::cout << "    + indexCount:  " << primitive->indexCount << "\n";
        std::cout << "    + material:    " << primitive->material->name << "\n";
    }
}
