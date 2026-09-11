#include <cstdio>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include "frustum_culling.h"

const std::string planetRelPath = "obj/sphere.obj";
const std::string planetTextureRelPath = "texture/miscellaneous/planet_Quom1200.png";

const std::string asternoldRelPath = "obj/rock.obj";
const std::string asternoldTextureRelPath = "texture/miscellaneous/Rock-Texture-Surface.jpg";

const std::string aabbVsRelPath = "shader/bonus2/aabb.vert";
const std::string aabbInstancedVsRelPath = "shader/bonus2/aabb_instanced.vert";
const std::string aabbFsRelPath = "shader/bonus2/aabb.frag";

const std::string lambertVsRelPath = "shader/bonus2/lambert.vert";
const std::string lambertInstancedVsRelPath = "shader/bonus2/lambert_instanced.vert";
const std::string lambertFsRelPath = "shader/bonus2/lambert.frag";

const std::string frustumCullingVsRelPath = "shader/bonus2/frustum_culling.vert";
#ifdef USE_GLES
const std::string frustumCullingFsRelPath = "shader/bonus2/frustum_culling.frag";
#endif

FrustumCulling::FrustumCulling(const Options& options) : Application(options) {
    // init model matrices
    initModelMatrices();

    // init model
    m_planet.reset(new Model(getAssetFullPath(planetRelPath)));
    m_planet->transform.scale = glm::vec3(10.0f, 10.0f, 10.0f);

    m_asternoid.reset(new Model(getAssetFullPath(asternoldRelPath)));
    m_instancedAsternoids.reset(
        new InstancedModel(getAssetFullPath(asternoldRelPath), m_modelMatrices));

    // init textures
    auto planetTexture = std::make_shared<ImageTexture2D>(getAssetFullPath(planetTextureRelPath));
    auto asternoidTexture =
        std::make_shared<ImageTexture2D>(getAssetFullPath(asternoldTextureRelPath));

    // init materials
    m_lineMaterial.reset(new LineMaterial);
    m_lineMaterial->color = glm::vec3(0.0f, 1.0f, 0.0f);
    m_lineMaterial->width = 1.0f;

    m_planetMaterial.reset(new LambertMaterial);
    m_planetMaterial->kd = glm::vec3(1.0f, 1.0f, 1.0f);
    m_planetMaterial->mapKd = planetTexture;

    m_asternoidMaterial.reset(new LambertMaterial);
    m_asternoidMaterial->kd = glm::vec3(1.0f, 1.0f, 1.0f);
    m_asternoidMaterial->mapKd = asternoidTexture;

    // init shaders
    initShaders();

    // init camera
    m_camera.reset(new PerspectiveCamera(
        glm::radians(45.0f), 1.0f * m_windowWidth / m_windowHeight, 0.1f, 1000.0f));

    m_camera->transform.position = glm::vec3(0.0f, 25.0f, 100.0f);
    m_camera->transform.rotation =
        glm::angleAxis(-glm::radians(20.0f), m_camera->transform.getRight());

    // init light
    m_light.reset(new DirectionalLight());
    m_light->transform.rotation =
        glm::angleAxis(glm::radians(45.0f), glm::normalize(glm::vec3(-1.0f, -2.0f, -1.0f)));

    // init visible array
    m_visibles.resize(m_amount, 1);

    // init gpu frustum culling resources
    initGPUCullingResources();

    // init indirect draw resources
    m_indirectDrawCmds.reserve(m_amount);
    glGenBuffers(1, &m_indirectBuffer);

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

FrustumCulling::~FrustumCulling() {
    // destroy GPU frustum culling resources
    if (m_transformFeedback) {
        glDeleteTransformFeedbacks(1, &m_transformFeedback);
        m_transformFeedback = 0;
    }

    if (m_transformFeedbackResultBuffer) {
        glDeleteBuffers(1, &m_transformFeedbackResultBuffer);
        m_transformFeedbackResultBuffer = 0;
    }

    // destroy indirect draw resources
    if (m_indirectBuffer) {
        glDeleteBuffers(1, &m_indirectBuffer);
        m_indirectBuffer = 0;
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

void FrustumCulling::initModelMatrices() {
    constexpr float radius = 50.0f;
    constexpr float offset = 10.0f;
    for (int i = 0; i < m_amount; ++i) {
        glm::mat4 model(1.0f);
        // translate
        float angle = (float)i / (float)m_amount * 360.0f;
        float displacement = (rand() % (int)(2 * offset * 100)) / 100.0f - offset;
        float x = sin(angle) * radius + displacement;
        displacement = (rand() % (int)(2 * offset * 100)) / 100.0f - offset;
        float y = displacement * 0.2f;
        displacement = (rand() % (int)(2 * offset * 100)) / 100.0f - offset;
        float z = cos(angle) * radius + displacement;
        model = glm::translate(model, glm::vec3(x, y, z));

        // scale
        float scale = (rand() % 20) / 100.0f + 0.05f;
        model = glm::scale(model, glm::vec3(scale));

        // rotate
        float rotAngle = 1.0f * (rand() % 360);
        model = glm::rotate(model, rotAngle, glm::vec3(0.4f, 0.6f, 0.8f));

        m_modelMatrices.push_back(model);
    }
}

void FrustumCulling::initShaders() {
    const char* version =
#ifdef USE_GLES
        "300 es"
#else
        "330 core"
#endif
        ;

    m_lineShader.reset(new GLSLProgram);
    m_lineShader->attachVertexShaderFromFile(getAssetFullPath(aabbVsRelPath), version);
    m_lineShader->attachFragmentShaderFromFile(getAssetFullPath(aabbFsRelPath), version);
    m_lineShader->link();

    m_lineInstancedShader.reset(new GLSLProgram);
    m_lineInstancedShader->attachVertexShaderFromFile(
        getAssetFullPath(aabbInstancedVsRelPath), version);
    m_lineInstancedShader->attachFragmentShaderFromFile(getAssetFullPath(aabbFsRelPath), version);
    m_lineInstancedShader->link();

    m_lambertShader.reset(new GLSLProgram);
    m_lambertShader->attachVertexShaderFromFile(getAssetFullPath(lambertVsRelPath), version);
    m_lambertShader->attachFragmentShaderFromFile(getAssetFullPath(lambertFsRelPath), version);
    m_lambertShader->link();

    m_lambertInstancedShader.reset(new GLSLProgram);
    m_lambertInstancedShader->attachVertexShaderFromFile(
        getAssetFullPath(lambertInstancedVsRelPath), version);
    m_lambertInstancedShader->attachFragmentShaderFromFile(
        getAssetFullPath(lambertFsRelPath), version);
    m_lambertInstancedShader->link();

    // create frustum culling shader
    // TODO: Modify the frustum_culling.vert code to achieve GPU frustum culling
    m_frustumCullingShader.reset(new GLSLProgram);
    m_frustumCullingShader->attachVertexShaderFromFile(
        getAssetFullPath(frustumCullingVsRelPath), version);
    m_frustumCullingShader->setTransformFeedbackVaryings({"visible"}, GL_INTERLEAVED_ATTRIBS);
#ifdef USE_GLES
    // GLSL/ES program must link to a fragment shader, while OpenGL need not.
    m_frustumCullingShader->attachFragmentShaderFromFile(
        getAssetFullPath(frustumCullingFsRelPath), version);
#endif
    m_frustumCullingShader->link();
}

void FrustumCulling::initGPUCullingResources() {
    // create transform feedback
    glGenTransformFeedbacks(1, &m_transformFeedback);
    glBindTransformFeedback(GL_TRANSFORM_FEEDBACK, m_transformFeedback);

    // create transform feedback result buffer
    glGenBuffers(1, &m_transformFeedbackResultBuffer);
    glBindBuffer(GL_TRANSFORM_FEEDBACK_BUFFER, m_transformFeedbackResultBuffer);
    glBufferData(GL_TRANSFORM_FEEDBACK_BUFFER, m_amount * sizeof(int), nullptr, GL_DYNAMIC_DRAW);

    glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, m_transformFeedbackResultBuffer);
    glBindTransformFeedback(GL_TRANSFORM_FEEDBACK, 0);
}

void FrustumCulling::handleInput() {
    if (m_input.keyboard.keyStates[GLFW_KEY_ESCAPE] != GLFW_RELEASE) {
        glfwSetWindowShouldClose(m_window, true);
        return;
    }

    if (m_input.keyboard.keyStates[GLFW_KEY_W] != GLFW_RELEASE) {
        m_camera->transform.position +=
            m_camera->transform.getFront() * m_cameraMoveSpeed * m_deltaTime;
    }

    if (m_input.keyboard.keyStates[GLFW_KEY_A] != GLFW_RELEASE) {
        m_camera->transform.position -=
            m_camera->transform.getRight() * m_cameraMoveSpeed * m_deltaTime;
    }

    if (m_input.keyboard.keyStates[GLFW_KEY_S] != GLFW_RELEASE) {
        m_camera->transform.position -=
            m_camera->transform.getFront() * m_cameraMoveSpeed * m_deltaTime;
    }

    if (m_input.keyboard.keyStates[GLFW_KEY_D] != GLFW_RELEASE) {
        m_camera->transform.position +=
            m_camera->transform.getRight() * m_cameraMoveSpeed * m_deltaTime;
    }

#ifndef USE_GLES
    if (glMultiDrawElementsIndirect == nullptr) {
        m_indirectDrawEnabled = false;
    }
#endif
}

void FrustumCulling::renderFrame() {
    glClearColor(m_clearColor.r, m_clearColor.g, m_clearColor.b, m_clearColor.a);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);

    const Frustum frustum = m_camera->getFrustum();
    const glm::mat4 projection = m_camera->getProjectionMatrix();
    const glm::mat4 view = m_camera->getViewMatrix();

    // draw planet
    if (frustum.intersect(m_planet->getBoundingBox(), m_planet->transform.getLocalMatrix())) {
        m_lambertShader->use();
        m_lambertShader->setUniformMat4("projection", projection);
        m_lambertShader->setUniformMat4("view", view);
        m_lambertShader->setUniformMat4("model", m_planet->transform.getLocalMatrix());
        m_lambertShader->setUniformVec3("light.direction", m_light->transform.getFront());
        m_lambertShader->setUniformVec3("light.color", m_light->color);
        m_lambertShader->setUniformFloat("light.intensity", m_light->intensity);
        m_lambertShader->setUniformVec3("material.kd", m_planetMaterial->kd);
        glActiveTexture(GL_TEXTURE0);
        m_planetMaterial->mapKd->bind();

        m_planet->draw();
    }

    // draw planet aabb
    if (m_showBoundingBox) {
        m_lineShader->use();
        m_lineShader->setUniformMat4("projection", projection);
        m_lineShader->setUniformMat4("view", view);
        m_lineShader->setUniformMat4("model", m_planet->transform.getLocalMatrix());
        m_lineShader->setUniformVec3("material.color", m_lineMaterial->color);
        glLineWidth(m_lineMaterial->width);

        m_planet->drawBoundingBox();
    }

    // test visiblity
    // results will be stored in std::vector<int> m_visibles
    const BoundingBox box = m_asternoid->getBoundingBox();
    switch (m_method) {
    case Method::CPU:
        for (int i = 0; i < m_amount; ++i) {
            m_visibles[i] = static_cast<int>(frustum.intersect(box, m_modelMatrices[i]));
        }
        break;
    case Method::GPU:
        // TODO: use the transform feedback to perform GPU frustum culling
        // write your code here
        // ------------------------------------------------------------------
        // m_frustumCullingShader->use();
        // ------------------------------------------------------------------

        break;
    }

    if (m_indirectDrawEnabled) {
        renderAsternoidsIndirect();
    } else {
        renderAsternoids();
    }

    // draw ui
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    const auto flags = ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings;

    if (!ImGui::Begin("Control Panel", nullptr, flags)) {
        ImGui::End();
    } else {
        ImGui::Text("render method");
        ImGui::Separator();
        ImGui::RadioButton("CPU", (int*)&m_method, (int)(Method::CPU));
        ImGui::RadioButton("GPU", (int*)&m_method, (int)(Method::GPU));

        ImGui::Checkbox("draw indirect", (bool*)&m_indirectDrawEnabled);
        ImGui::Checkbox("show bounding box", (bool*)&m_showBoundingBox);
        ImGui::NewLine();

        float fraction = 1.0f * m_drawAsternoidCount / m_amount;
        std::string fracInfo = std::to_string(m_drawAsternoidCount) + "/" + std::to_string(m_amount);
        ImGui::Text("visible fraction");
        ImGui::ProgressBar(fraction, ImVec2(0.0f, 0.0f), fracInfo.c_str());
        ImGui::NewLine();

        std::string fpsInfo = "avg fps: " + std::to_string(m_fpsIndicator.getAverageFrameRate());
        ImGui::Text("%s", fpsInfo.c_str());
        ImGui::PlotLines(
            "", m_fpsIndicator.getDataPtr(), m_fpsIndicator.getSize(), 0, nullptr, 0.0f,
            std::numeric_limits<float>::max(), ImVec2(240.0f, 50.0f));

        ImGui::End();
    }

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void FrustumCulling::renderAsternoids() {
    m_drawAsternoidCount = 0;

    const glm::mat4 projection = m_camera->getProjectionMatrix();
    const glm::mat4 view = m_camera->getViewMatrix();

    m_lambertShader->use();
    m_lambertShader->setUniformMat4("projection", projection);
    m_lambertShader->setUniformMat4("view", view);
    m_lambertShader->setUniformVec3("light.direction", m_light->transform.getFront());
    m_lambertShader->setUniformVec3("light.color", m_light->color);
    m_lambertShader->setUniformFloat("light.intensity", m_light->intensity);
    m_lambertShader->setUniformVec3("material.kd", m_asternoidMaterial->kd);
    glActiveTexture(GL_TEXTURE0);
    m_asternoidMaterial->mapKd->bind();

    for (int i = 0; i < m_amount; ++i) {
        if (m_visibles[i]) {
            m_lambertShader->setUniformMat4("model", m_modelMatrices[i]);
            m_asternoid->draw();
            ++m_drawAsternoidCount;
        }
    }

    if (m_showBoundingBox) {
        m_lineShader->use();
        m_lineShader->setUniformMat4("projection", projection);
        m_lineShader->setUniformMat4("view", view);
        glLineWidth(m_lineMaterial->width);
        m_lineShader->setUniformVec3("material.color", m_lineMaterial->color);

        for (int i = 0; i < m_amount; ++i) {
            m_lineShader->setUniformMat4("model", m_modelMatrices[i]);
            m_asternoid->drawBoundingBox();
        }
    }
}

void FrustumCulling::renderAsternoidsIndirect() {
#ifdef USE_GLES
    m_indirectDrawEnabled = false;
    std::cerr << "indirect draw is not supported with OpenGL/ES" << std::endl;
#else
    m_drawAsternoidCount = 0;

    m_indirectDrawCmds.clear();

    const glm::mat4 projection = m_camera->getProjectionMatrix();
    const glm::mat4 view = m_camera->getViewMatrix();
    const uint32_t count = static_cast<uint32_t>(m_asternoid->getFaceCount() * 3);
    uint32_t instanceCount = 0;

    for (int i = 0; i < m_amount; ++i) {
        if (m_visibles[i]) {
            ++instanceCount;
            ++m_drawAsternoidCount;
        } else {
            m_indirectDrawCmds.push_back({count, instanceCount, 0, 0, i - instanceCount});
            instanceCount = 0;
        }
    }

    if (instanceCount > 0) {
        m_indirectDrawCmds.push_back({count, instanceCount, 0, 0, m_amount - instanceCount});
    }

    m_lambertInstancedShader->use();
    m_lambertInstancedShader->setUniformMat4("projection", projection);
    m_lambertInstancedShader->setUniformMat4("view", view);
    m_lambertInstancedShader->setUniformVec3("light.direction", m_light->transform.getFront());
    m_lambertInstancedShader->setUniformVec3("light.color", m_light->color);
    m_lambertInstancedShader->setUniformFloat("light.intensity", m_light->intensity);
    m_lambertInstancedShader->setUniformVec3("material.kd", m_asternoidMaterial->kd);
    glActiveTexture(GL_TEXTURE0);
    m_asternoidMaterial->mapKd->bind();

    glBindBuffer(GL_DRAW_INDIRECT_BUFFER, m_indirectBuffer);
    glBufferData(
        GL_DRAW_INDIRECT_BUFFER, m_indirectDrawCmds.size() * sizeof(DrawElementsIndirectCommand),
        m_indirectDrawCmds.data(), GL_STREAM_DRAW);

    glBindVertexArray(m_instancedAsternoids->getVao());

    glMultiDrawElementsIndirect(
        GL_TRIANGLES, GL_UNSIGNED_INT, 0, static_cast<GLsizei>(m_indirectDrawCmds.size()), 0);

    glBindBuffer(GL_DRAW_INDIRECT_BUFFER, 0);
    glBindVertexArray(0);

    if (m_showBoundingBox) {
        for (auto& cmd : m_indirectDrawCmds) {
            cmd.count = 24;
        }

        m_lineInstancedShader->use();
        m_lineInstancedShader->setUniformMat4("projection", projection);
        m_lineInstancedShader->setUniformMat4("view", view);
        glLineWidth(m_lineMaterial->width);
        m_lineInstancedShader->setUniformVec3("material.color", m_lineMaterial->color);

        glBindBuffer(GL_DRAW_INDIRECT_BUFFER, m_indirectBuffer);
        glBufferSubData(
            GL_DRAW_INDIRECT_BUFFER, 0,
            m_indirectDrawCmds.size() * sizeof(DrawElementsIndirectCommand),
            m_indirectDrawCmds.data());

        glBindVertexArray(m_instancedAsternoids->getBoundingBoxVao());

        glMultiDrawElementsIndirect(
            GL_LINES, GL_UNSIGNED_INT, 0, static_cast<GLsizei>(m_indirectDrawCmds.size()), 0);

        glBindBuffer(GL_DRAW_INDIRECT_BUFFER, 0);

        glBindVertexArray(0);
    }
#endif
}