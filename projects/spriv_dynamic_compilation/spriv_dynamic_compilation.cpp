#include "spriv_dynamic_compilation.h"

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include "program_manager.h"

SprivDynamicCompilation::SprivDynamicCompilation(const Options& options) : Application(options) {
    _dirLight.reset(new DirectionalLight);
    _dirLight->intensity = 1.0f;
    _dirLight->transform.rotation =
        glm::angleAxis(glm::radians(45.0f), glm::normalize(glm::vec3(-1.0f)));

    _camera.reset(new PerspectiveCamera(
        glm::radians(50.0f), 1.0f * _windowWidth / _windowHeight, 0.1f, 1000.0f));
    _camera->transform.position.z = 10.0f;

    initMaterial();

    _uboCamera = std::make_unique<UniformBuffer>(144, GL_DYNAMIC_DRAW);
    _uboCamera->setOffset("projection", 0);
    _uboCamera->setOffset("view", 64);
    _uboCamera->setOffset("viewPos", 128);
    _uboCamera->setBindingPoint(0);

    _model.reset(new Model(getAssetFullPath("obj/bunny.obj")));

    // init imGUI
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    (void)io;

    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(_window, true);
    ImGui_ImplOpenGL3_Init();

    checkGLErrors();
}

void SprivDynamicCompilation::initMaterial() {
    _programManager.reset(new ProgramManager);

    std::vector<ProgramManager::MarcoDefinition> macros{
        { "OUTPUT_RED_CHANNAL", "0" },
    };

    std::vector<ProgramManager::ShaderSource> shaderSources{
        {
            ShaderModule::Stage::Vertex,
            getAssetFullPath("shader/spriv_dynamic_compilation/lambert.vert"),
        },
        { 
            ShaderModule::Stage::Fragment, 
            getAssetFullPath("shader/spriv_dynamic_compilation/lambert.frag"), 
            macros
        }
    };

    _lambertProgram = _programManager->create(shaderSources);
    _lambertMaterial = std::make_unique<Material>(_lambertProgram);
}

void SprivDynamicCompilation::handleInput() {
    if (_input.keyboard.keyStates[GLFW_KEY_ESCAPE] != GLFW_RELEASE) {
        glfwSetWindowShouldClose(_window, true);
        return;
    }
}

void SprivDynamicCompilation::renderFrame() {
    showFpsInWindowTitle();

    glClearColor(_clearColor.r, _clearColor.g, _clearColor.b, _clearColor.a);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);

    _uboCamera->update("projection", _camera->getProjectionMatrix());
    _uboCamera->update("view", _camera->getViewMatrix());

    _lambertProgram->use();
    _lambertProgram->setUniform(0, _model->transform.getLocalMatrix());
    _lambertProgram->setUniform(1, glm::vec3(0.8f));
    _lambertProgram->setUniform(2, _dirLight->transform.getFront());
    _lambertProgram->setUniform(3, _dirLight->intensity);
    _lambertProgram->setUniform(4, _dirLight->color);

    _model->draw();

    renderUI();
}

void SprivDynamicCompilation::renderUI() {
    // draw ui elements
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    const auto flags = ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings;

    if (!ImGui::Begin("Control Panel", nullptr, flags)) {
        ImGui::End();
    }
    else {
        ImGui::Text("directional light");
        ImGui::Separator();
        ImGui::SliderFloat("intensity", &_dirLight->intensity, 0.0f, 1.0f);
        ImGui::ColorEdit3("color", (float*)&_dirLight->color);
        ImGui::NewLine();

        ImGui::End();
    }

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}