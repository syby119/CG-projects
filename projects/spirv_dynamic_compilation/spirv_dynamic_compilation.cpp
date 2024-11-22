#include "spirv_dynamic_compilation.h"

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include "program_manager.h"

SpirvDynamicCompilation::SpirvDynamicCompilation(const Options& options) : Application(options) {
    _dirLight.reset(new DirectionalLight);
    _dirLight->intensity = 1.0f;
    _dirLight->transform.rotation =
        glm::angleAxis(glm::radians(45.0f), glm::normalize(glm::vec3(-1.0f)));

    _camera.reset(new PerspectiveCamera(
        glm::radians(50.0f), 1.0f * _windowWidth / _windowHeight, 0.1f, 1000.0f));
    _camera->transform.position.z = 4.0f;

    _uboCamera = std::make_unique<UniformBuffer>(144, GL_DYNAMIC_DRAW);
    _uboCamera->setOffset("projection", 0);
    _uboCamera->setOffset("view", 64);
    _uboCamera->setOffset("viewPos", 128);
    _uboCamera->setBindingPoint(0);

    _uboLights = std::make_unique<UniformBuffer>(144, GL_DYNAMIC_DRAW);
    _uboLights->setOffset("numDirLights", 0);
    for (size_t i = 0; i < 4; ++i) {
        std::string lightPrefix = "dirLights[" + std::to_string(i) + "]";
        _uboLights->setOffset(lightPrefix + ".direction", 16 + 32 * i);
        _uboLights->setOffset(lightPrefix + ".intensity", 28 + 32 * i);
        _uboLights->setOffset(lightPrefix + ".color", 32 + 32 * i);
    }
    _uboLights->setBindingPoint(1);

    _model.reset(new Model(getAssetFullPath("obj/sphere.obj")));
    _texture.reset(new ImageTexture2D(getAssetFullPath("texture/miscellaneous/earthmap.jpg")));

    initMaterial();

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

void SpirvDynamicCompilation::initMaterial() {
    _programManager.reset(new ProgramManager);

    std::vector<ProgramManager::MarcoDefinition> macros{
        { "OUTPUT_RED_CHANNAL", "0" },
    };

    std::vector<ProgramManager::ShaderSource> shaderSources{
        {
            ShaderModule::Stage::Vertex,
            getAssetFullPath("shader/spirv_dynamic_compilation/lambert.vert"),
        },
        {
            ShaderModule::Stage::Fragment,
            getAssetFullPath("shader/spirv_dynamic_compilation/lambert.frag"),
            macros
        }
    };

    _lambertProgram = _programManager->create(shaderSources);
    _lambertMaterial = std::make_unique<Material>(_lambertProgram);
    _lambertMaterial->set("albedo", glm::vec3(0.8f));
    _lambertMaterial->set("albedoMap", _texture);

    _lambertProgram->printResourceInfos();
}

void SpirvDynamicCompilation::handleInput() {
    if (_input.keyboard.keyStates[GLFW_KEY_ESCAPE] != GLFW_RELEASE) {
        glfwSetWindowShouldClose(_window, true);
        return;
    }
}

void SpirvDynamicCompilation::renderFrame() {
    showFpsInWindowTitle();

    glClearColor(_clearColor.r, _clearColor.g, _clearColor.b, _clearColor.a);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);

    _uboCamera->update("projection", _camera->getProjectionMatrix());
    _uboCamera->update("view", _camera->getViewMatrix());

    _uboLights->update("numDirLights", 1);
    _uboLights->update("dirLights[0].direction", _dirLight->transform.getFront());
    _uboLights->update("dirLights[0].intensity", _dirLight->intensity);
    _uboLights->update("dirLights[0].color", _dirLight->color);

    auto program{ _lambertMaterial->getProgram() };
    program->use();
    _lambertMaterial->upload();
    _lambertMaterial->getProgram()->setUniform(
        program->getUniformVarLocation("model"), _model->transform.getLocalMatrix());

    _model->draw();

    renderUI();
}

void SpirvDynamicCompilation::renderUI() {
    // draw ui elements
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    const auto flags = ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings;

    if (!ImGui::Begin("Control Panel", nullptr, flags)) {
        ImGui::End();
    }
    else {
        renderLightUI();
        ImGui::NewLine();

        renderMaterialUI();

        ImGui::End();
    }

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void SpirvDynamicCompilation::renderLightUI() {
    ImGui::TextUnformatted("directional light");
    ImGui::Separator();
    ImGui::SliderFloat("intensity", &_dirLight->intensity, 0.0f, 1.0f);
    ImGui::ColorEdit3("color", (float*)&_dirLight->color);
}

void SpirvDynamicCompilation::renderMaterialUI() {
    ImGui::TextUnformatted("material");
    ImGui::Separator();

    for (auto& [name, attrInfo] : _lambertMaterial->getArributeInfos()) {
        ImGui::TextUnformatted(name.c_str());

        auto label{ "##" + name };
        switch (attrInfo.type) {
        case GLProgram::VarType::Bool:
            ImGui::Checkbox(label.c_str(), const_cast<bool*>(std::get_if<bool>(&attrInfo.value)));
            break;
        case GLProgram::VarType::Int:
            ImGui::DragInt(label.c_str(), const_cast<int*>(std::get_if<int>(&attrInfo.value)), 0.01f);
            break;
        case GLProgram::VarType::Float:
            ImGui::DragFloat(label.c_str(), (float*)(std::get_if<float>(&attrInfo.value)), 0.01f);
            break;
        case GLProgram::VarType::Vec2:
            ImGui::DragFloat2(label.c_str(), (float*)(std::get_if<glm::vec2>(&attrInfo.value)), 0.01f);
            break;
        case GLProgram::VarType::Vec3:
            if (attrInfo.isColor) {
                ImGui::ColorEdit3(label.c_str(), (float*)(std::get_if<glm::vec3>(&attrInfo.value)));
            }
            else {
                ImGui::DragFloat3(label.c_str(), (float*)(std::get_if<glm::vec3>(&attrInfo.value)), 0.01f);
            }
            break;
        case GLProgram::VarType::Vec4:
            if (attrInfo.isColor) {
                ImGui::ColorEdit4(label.c_str(), (float*)(std::get_if<glm::vec4>(&attrInfo.value)));
            }
            else {
                ImGui::DragFloat4(label.c_str(), (float*)(std::get_if<glm::vec4>(&attrInfo.value)), 0.01f);
            }
            break;
        }
    }

    for (auto const& [name, texInfo] : _lambertMaterial->getTextureInfos()) {
        ImGui::TextUnformatted(name.c_str());
        ImGui::Image((void*)(uint64_t)texInfo.texture->getHandle(),
            ImVec2(256, 256), ImVec2{ 0, 1 }, ImVec2{ 1, 0 });
    }
}