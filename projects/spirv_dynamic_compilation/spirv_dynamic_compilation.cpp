#include "spirv_dynamic_compilation.h"

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include "program_manager.h"

SpirvDynamicCompilation::SpirvDynamicCompilation(const Options& options) : Application(options) {
    m_dirLight.reset(new DirectionalLight);
    m_dirLight->intensity = 1.0f;
    m_dirLight->transform.rotation =
        glm::angleAxis(glm::radians(45.0f), glm::normalize(glm::vec3(-1.0f)));

    m_camera.reset(new PerspectiveCamera(
        glm::radians(50.0f), 1.0f * m_windowWidth / m_windowHeight, 0.1f, 1000.0f));
    m_camera->transform.position.z = 4.0f;

    m_uboCamera = std::make_unique<UniformBuffer>(144, GL_DYNAMIC_DRAW);
    m_uboCamera->setOffset("projection", 0);
    m_uboCamera->setOffset("view", 64);
    m_uboCamera->setOffset("viewPos", 128);
    m_uboCamera->setBindingPoint(0);

    m_uboLights = std::make_unique<UniformBuffer>(144, GL_DYNAMIC_DRAW);
    m_uboLights->setOffset("numDirLights", 0);
    for (size_t i = 0; i < 4; ++i) {
        std::string lightPrefix = "dirLights[" + std::to_string(i) + "]";
        m_uboLights->setOffset(lightPrefix + ".direction", 16 + 32 * i);
        m_uboLights->setOffset(lightPrefix + ".intensity", 28 + 32 * i);
        m_uboLights->setOffset(lightPrefix + ".color", 32 + 32 * i);
    }
    m_uboLights->setBindingPoint(1);

    m_model.reset(new Model(getAssetFullPath("obj/sphere.obj")));
    m_texture.reset(new ImageTexture2D(getAssetFullPath("texture/miscellaneous/earthmap.jpg")));

    initMaterial();

    // init imGUI
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    (void)io;

    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(m_window, true);
    ImGui_ImplOpenGL3_Init();

    checkGLErrors();
}

void SpirvDynamicCompilation::initMaterial() {
    m_programManager.reset(new ProgramManager);

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

    m_lambertProgram = m_programManager->create(shaderSources);
    m_lambertMaterial = std::make_unique<Material>(m_lambertProgram);
    m_lambertMaterial->set("albedo", glm::vec3(0.8f));
    m_lambertMaterial->set("albedoMap", m_texture);

    m_lambertProgram->printResourceInfos();
}

void SpirvDynamicCompilation::handleInput() {
    if (m_input.keyboard.keyStates[GLFW_KEY_ESCAPE] != GLFW_RELEASE) {
        glfwSetWindowShouldClose(m_window, true);
        return;
    }
}

void SpirvDynamicCompilation::renderFrame() {
    showFpsInWindowTitle();

    glClearColor(m_clearColor.r, m_clearColor.g, m_clearColor.b, m_clearColor.a);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);

    m_uboCamera->update("projection", m_camera->getProjectionMatrix());
    m_uboCamera->update("view", m_camera->getViewMatrix());

    m_uboLights->update("numDirLights", 1);
    m_uboLights->update("dirLights[0].direction", m_dirLight->transform.getFront());
    m_uboLights->update("dirLights[0].intensity", m_dirLight->intensity);
    m_uboLights->update("dirLights[0].color", m_dirLight->color);

    auto program{ m_lambertMaterial->getProgram() };
    program->use();
    m_lambertMaterial->upload();
    m_lambertMaterial->getProgram()->setUniform(
        program->getUniformVarLocation("model"), m_model->transform.getLocalMatrix());

    m_model->draw();

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
    ImGui::SliderFloat("intensity", &m_dirLight->intensity, 0.0f, 1.0f);
    ImGui::ColorEdit3("color", (float*)&m_dirLight->color);
}

void SpirvDynamicCompilation::renderMaterialUI() {
    ImGui::TextUnformatted("material");
    ImGui::Separator();

    for (auto& [name, attrInfo] : m_lambertMaterial->getArributeInfos()) {
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

    for (auto const& [name, texInfo] : m_lambertMaterial->getTextureInfos()) {
        ImGui::TextUnformatted(name.c_str());
        ImGui::Image((void*)(uint64_t)texInfo.texture->getHandle(),
            ImVec2(256, 256), ImVec2{ 0, 1 }, ImVec2{ 1, 0 });
    }
}