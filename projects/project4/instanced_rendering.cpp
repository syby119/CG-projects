#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include "instanced_rendering.h"

const std::string planetRelPath = "obj/sphere.obj";
const std::string asternoidRelPath = "obj/rock.obj";

InstancedRendering::InstancedRendering(const Options& options) : Application(options) {
    // import models
    m_planet.reset(new Model(getAssetFullPath(planetRelPath)));
    m_planet->transform.scale = glm::vec3(10.0f, 10.0f, 10.0f);

    m_asternoid.reset(new Model(getAssetFullPath(asternoidRelPath)));

    // init camera
    m_camera.reset(new PerspectiveCamera(
        glm::radians(45.0f), 1.0f * m_windowWidth / m_windowHeight, 0.1f, 10000.0f));

    m_camera->transform.position = glm::vec3(0.0f, 25.0f, 100.0f);
    m_camera->transform.rotation =
        glm::angleAxis(-glm::radians(20.0f), m_camera->transform.getRight());

    /* shader issues */
    initShaders();

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

    // TODO: configure the instanced buffer and transfer the matrix data to GPU
    // write your code here
    // ---------------------------------------------------------
    // glXXX(m_instanceBuffer); ...
    // ---------------------------------------------------------

    // init imGUI
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    (void)io;

    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(m_window, true);
    ImGui_ImplOpenGL3_Init();
}

InstancedRendering::~InstancedRendering() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

void InstancedRendering::initShaders() {
    const char* planetVsCode =
        "#version 330 core\n"
        "layout(location = 0) in vec3 aPosition;\n"
        "uniform mat4 projection;\n"
        "uniform mat4 view;\n"
        "uniform mat4 model;\n"
        "void main() {\n"
        "    gl_Position = projection * view * model * vec4(aPosition, 1.0f);\n"
        "}\n";

    const char* planetFsCode =
        "#version 330 core\n"
        "out vec4 FragColor;\n"
        "void main() {\n"
        "    FragColor = vec4(0.5f, 0.5f, 0.5f, 1.0f);\n"
        "}\n";

    m_planetShader.reset(new GLSLProgram);
    m_planetShader->attachVertexShader(planetVsCode);
    m_planetShader->attachFragmentShader(planetFsCode);
    m_planetShader->link();

    const char* asternoidVsCode =
        "#version 330 core\n"
        "layout(location = 0) in vec3 aPosition;\n"
        "uniform mat4 projection;\n"
        "uniform mat4 view;\n"
        "uniform mat4 model;\n"
        "void main() {\n"
        "    gl_Position = projection * view * model * vec4(aPosition, 1.0f);\n"
        "}\n";

    const char* asternoidInstancedVsCode =
        "#version 330 core\n"
        "layout(location = 0) in vec3 aPosition;\n"
        "layout(location = 3) in mat4 aInstanceMatrix;\n"
        "uniform mat4 projection;\n"
        "uniform mat4 view;\n"
        "void main() {\n"
        "    gl_Position = projection * view * aInstanceMatrix * vec4(aPosition, 1.0f);\n"
        "}\n";

    const char* asternoidFsCode =
        "#version 330 core\n"
        "out vec4 FragColor;\n"
        "void main() {\n"
        "    FragColor = vec4(0.8f, 0.8f, 0.8f, 1.0f);\n"
        "}\n";

    m_asternoidShader.reset(new GLSLProgram);
    m_asternoidShader->attachVertexShader(asternoidVsCode);
    m_asternoidShader->attachFragmentShader(asternoidFsCode);
    m_asternoidShader->link();

    m_asternoidInstancedShader.reset(new GLSLProgram);
    m_asternoidInstancedShader->attachVertexShader(asternoidInstancedVsCode);
    m_asternoidInstancedShader->attachFragmentShader(asternoidFsCode);
    m_asternoidInstancedShader->link();
}

void InstancedRendering::handleInput() {
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
}

void InstancedRendering::renderFrame() {
    glClearColor(m_clearColor.r, m_clearColor.g, m_clearColor.b, m_clearColor.a);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);

    if (m_wireframe) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    } else {
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }

    // get camera properties
    glm::mat4 projection = m_camera->getProjectionMatrix();
    glm::mat4 view = m_camera->getViewMatrix();

    // draw planet
    m_planetShader->use();
    m_planetShader->setUniformMat4("model", m_planet->transform.getLocalMatrix());
    m_planetShader->setUniformMat4("view", view);
    m_planetShader->setUniformMat4("projection", projection);
    m_planet->draw();

    // draw asternoids
    switch (m_renderMode) {
    case RenderMode::Ordinary:
        m_asternoidShader->use();
        m_asternoidShader->setUniformMat4("view", view);
        m_asternoidShader->setUniformMat4("projection", projection);
        for (int i = 0; i < m_amount; ++i) {
            m_asternoidShader->setUniformMat4("model", m_modelMatrices[i]);
            m_asternoid->draw();
        }
        break;
    case RenderMode::Instanced:
        m_asternoidInstancedShader->use();
        m_asternoidInstancedShader->setUniformMat4("view", view);
        m_asternoidInstancedShader->setUniformMat4("projection", projection);

        // TODO: draw the asternoids by the instance rendering method
        // write your code here
        // ---------------------------------------------------------
        // ...
        // glDraw...(...);
        // ---------------------------------------------------------

        break;
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
        ImGui::RadioButton("ordinary rendering", (int*)&m_renderMode, (int)(RenderMode::Ordinary));
        ImGui::RadioButton("instanced rendering", (int*)&m_renderMode, (int)(RenderMode::Instanced));
        ImGui::Checkbox("wireframe", &m_wireframe);
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