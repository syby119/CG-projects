#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include "instanced_rendering.h"

const std::string planetRelPath = "obj/sphere.obj";
const std::string asternoidRelPath = "obj/rock.obj";

InstancedRendering::InstancedRendering(const Options& options) : Application(options) {
    // import models
    _planet.reset(new Model(getAssetFullPath(planetRelPath)));
    _planet->transform.scale = glm::vec3(10.0f, 10.0f, 10.0f);

    _asternoid.reset(new Model(getAssetFullPath(asternoidRelPath)));

    // init camera
    _camera.reset(new PerspectiveCamera(
        glm::radians(45.0f), 1.0f * _windowWidth / _windowHeight, 0.1f, 10000.0f));

    _camera->transform.position = glm::vec3(0.0f, 25.0f, 100.0f);
    _camera->transform.rotation =
        glm::angleAxis(-glm::radians(20.0f), _camera->transform.getRight());

    /* shader issues */
    initShaders();

    constexpr float radius = 50.0f;
    constexpr float offset = 10.0f;
    for (int i = 0; i < _amount; ++i) {
        glm::mat4 model(1.0f);
        // translate
        float angle = (float)i / (float)_amount * 360.0f;
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

        _modelMatrices.push_back(model);
    }

    // TODO: configure the instanced buffer and transfer the matrix data to GPU
    // write your code here
    // ---------------------------------------------------------
    // glXXX(_instanceBuffer); ...
    // ---------------------------------------------------------

    // init imGUI
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    (void)io;

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

InstancedRendering::~InstancedRendering() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

void InstancedRendering::initShaders() {
    const char* version =
#ifdef USE_GLES
        "300 es"
#else
        "330 core"
#endif
        ;

    const char* planetVsCode =
        "layout(location = 0) in vec3 aPosition;\n"
        "uniform mat4 projection;\n"
        "uniform mat4 view;\n"
        "uniform mat4 model;\n"
        "void main() {\n"
        "    gl_Position = projection * view * model * vec4(aPosition, 1.0f);\n"
        "}\n";

    const char* planetFsCode =
        "out vec4 FragColor;\n"
        "void main() {\n"
        "    FragColor = vec4(0.5f, 0.5f, 0.5f, 1.0f);\n"
        "}\n";

    _planetShader.reset(new GLSLProgram);
    _planetShader->attachVertexShader(planetVsCode, version);
    _planetShader->attachFragmentShader(planetFsCode, version);
    _planetShader->link();

    const char* asternoidVsCode =
        "layout(location = 0) in vec3 aPosition;\n"
        "uniform mat4 projection;\n"
        "uniform mat4 view;\n"
        "uniform mat4 model;\n"
        "void main() {\n"
        "    gl_Position = projection * view * model * vec4(aPosition, 1.0f);\n"
        "}\n";

    const char* asternoidInstancedVsCode =
        "layout(location = 0) in vec3 aPosition;\n"
        "layout(location = 3) in mat4 aInstanceMatrix;\n"
        "uniform mat4 projection;\n"
        "uniform mat4 view;\n"
        "void main() {\n"
        "    gl_Position = projection * view * aInstanceMatrix * vec4(aPosition, 1.0f);\n"
        "}\n";

    const char* asternoidFsCode =
        "out vec4 FragColor;\n"
        "void main() {\n"
        "    FragColor = vec4(0.8f, 0.8f, 0.8f, 1.0f);\n"
        "}\n";

    _asternoidShader.reset(new GLSLProgram);
    _asternoidShader->attachVertexShader(asternoidVsCode, version);
    _asternoidShader->attachFragmentShader(asternoidFsCode, version);
    _asternoidShader->link();

    _asternoidInstancedShader.reset(new GLSLProgram);
    _asternoidInstancedShader->attachVertexShader(asternoidInstancedVsCode, version);
    _asternoidInstancedShader->attachFragmentShader(asternoidFsCode, version);
    _asternoidInstancedShader->link();
}

void InstancedRendering::handleInput() {
    if (_input.keyboard.keyStates[GLFW_KEY_ESCAPE] != GLFW_RELEASE) {
        glfwSetWindowShouldClose(_window, true);
        return;
    }

    if (_input.keyboard.keyStates[GLFW_KEY_W] != GLFW_RELEASE) {
        _camera->transform.position +=
            _camera->transform.getFront() * _cameraMoveSpeed * _deltaTime;
    }

    if (_input.keyboard.keyStates[GLFW_KEY_A] != GLFW_RELEASE) {
        _camera->transform.position -=
            _camera->transform.getRight() * _cameraMoveSpeed * _deltaTime;
    }

    if (_input.keyboard.keyStates[GLFW_KEY_S] != GLFW_RELEASE) {
        _camera->transform.position -=
            _camera->transform.getFront() * _cameraMoveSpeed * _deltaTime;
    }

    if (_input.keyboard.keyStates[GLFW_KEY_D] != GLFW_RELEASE) {
        _camera->transform.position +=
            _camera->transform.getRight() * _cameraMoveSpeed * _deltaTime;
    }
}

void InstancedRendering::renderFrame() {
    glClearColor(_clearColor.r, _clearColor.g, _clearColor.b, _clearColor.a);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);

#ifndef USE_GLES
    if (_wireframe) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    } else {
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }
#endif

    // get camera properties
    glm::mat4 projection = _camera->getProjectionMatrix();
    glm::mat4 view = _camera->getViewMatrix();

    // draw planet
    _planetShader->use();
    _planetShader->setUniformMat4("model", _planet->transform.getLocalMatrix());
    _planetShader->setUniformMat4("view", view);
    _planetShader->setUniformMat4("projection", projection);
    _planet->draw();

    // draw asternoids
    switch (_renderMode) {
    case RenderMode::Ordinary:
        _asternoidShader->use();
        _asternoidShader->setUniformMat4("view", view);
        _asternoidShader->setUniformMat4("projection", projection);
        for (int i = 0; i < _amount; ++i) {
            _asternoidShader->setUniformMat4("model", _modelMatrices[i]);
            _asternoid->draw();
        }
        break;
    case RenderMode::Instanced:
        _asternoidInstancedShader->use();
        _asternoidInstancedShader->setUniformMat4("view", view);
        _asternoidInstancedShader->setUniformMat4("projection", projection);

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
        ImGui::RadioButton("ordinary rendering", (int*)&_renderMode, (int)(RenderMode::Ordinary));
        ImGui::RadioButton("instanced rendering", (int*)&_renderMode, (int)(RenderMode::Instanced));
#ifndef __EMSCRIPTEN__
        ImGui::Checkbox("wireframe", &_wireframe);
#endif
        ImGui::NewLine();

        std::string fpsInfo = "avg fps: " + std::to_string(_fpsIndicator.getAverageFrameRate());
        ImGui::Text("%s", fpsInfo.c_str());
        ImGui::PlotLines(
            "", _fpsIndicator.getDataPtr(), _fpsIndicator.getSize(), 0, nullptr, 0.0f,
            std::numeric_limits<float>::max(), ImVec2(240.0f, 50.0f));

        ImGui::End();
    }

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}