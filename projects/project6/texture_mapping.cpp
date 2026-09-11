#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include "texture_mapping.h"

const std::string modelRelPath = "obj/sphere.obj";

const std::string earthTextureRelPath = "texture/miscellaneous/earthmap.jpg";
const std::string planetTextureRelPath = "texture/miscellaneous/planet_Quom1200.png";

const std::vector<std::string> skyboxTextureRelPaths = {
    "texture/skybox/Right_Tex.jpg", "texture/skybox/Left_Tex.jpg",  "texture/skybox/Up_Tex.jpg",
    "texture/skybox/Down_Tex.jpg",  "texture/skybox/Front_Tex.jpg", "texture/skybox/Back_Tex.jpg"};

TextureMapping::TextureMapping(const Options& options) : Application(options) {
    // init model
    m_sphere.reset(new Model(getAssetFullPath(modelRelPath)));
    m_sphere->transform.scale = glm::vec3(3.0f, 3.0f, 3.0f);

    // init textures
    std::shared_ptr<Texture2D> earthTexture =
        std::make_shared<ImageTexture2D>(getAssetFullPath(earthTextureRelPath));
    std::shared_ptr<Texture2D> planetTexture =
        std::make_shared<ImageTexture2D>(getAssetFullPath(planetTextureRelPath));

    // init materials
    m_simpleMaterial.reset(new SimpleMaterial);
    m_simpleMaterial->mapKd = planetTexture;

    m_blendMaterial.reset(new BlendMaterial);
    m_blendMaterial->kds[0] = glm::vec3(1.0f, 1.0f, 1.0f);
    m_blendMaterial->kds[1] = glm::vec3(1.0f, 1.0f, 1.0f);
    m_blendMaterial->mapKds[0] = planetTexture;
    m_blendMaterial->mapKds[1] = earthTexture;
    m_blendMaterial->blend = 0.0f;

    m_checkerMaterial.reset(new CheckerMaterial);
    m_checkerMaterial->repeat = 10;
    m_checkerMaterial->colors[0] = glm::vec3(1.0f, 1.0f, 1.0f);
    m_checkerMaterial->colors[1] = glm::vec3(0.0f, 0.0f, 0.0f);

    // init skybox
    std::vector<std::string> skyboxTextureFullPaths;
    for (size_t i = 0; i < skyboxTextureRelPaths.size(); ++i) {
        skyboxTextureFullPaths.push_back(getAssetFullPath(skyboxTextureRelPaths[i]));
    }
    m_skybox.reset(new SkyBox(skyboxTextureFullPaths));

    // init camera
    m_camera.reset(new PerspectiveCamera(
        glm::radians(50.0f), 1.0f * m_windowWidth / m_windowHeight, 0.1f, 10000.0f));
    m_camera->transform.position.z = 10.0f;

    // init light
    m_light.reset(new DirectionalLight());
    m_light->transform.rotation =
        glm::angleAxis(glm::radians(45.0f), glm::normalize(glm::vec3(-1.0f, -2.0f, -1.0f)));

    // init shaders
    initSimpleShader();
    initBlendShader();
    initCheckerShader();

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

TextureMapping::~TextureMapping() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

void TextureMapping::initSimpleShader() {
    const char* version =
#ifdef USE_GLES
        "300 es"
#else
        "330 core"
#endif
        ;

    const char* vsCode =
        "layout(location = 0) in vec3 aPosition;\n"
        "layout(location = 1) in vec3 aNormal;\n"
        "layout(location = 2) in vec2 aTexCoord;\n"
        "out vec2 fTexCoord;\n"
        "uniform mat4 projection;\n"
        "uniform mat4 view;\n"
        "uniform mat4 model;\n"

        "void main() {\n"
        "    fTexCoord = aTexCoord;\n"
        "    gl_Position = projection * view * model * vec4(aPosition, 1.0f);\n"
        "}\n";

    const char* fsCode =
        "in vec2 fTexCoord;\n"
        "out vec4 color;\n"
        "uniform sampler2D mapKd;\n"
        "void main() {\n"
        "    color = texture(mapKd, fTexCoord);\n"
        "}\n";

    m_simpleShader.reset(new GLSLProgram);
    m_simpleShader->attachVertexShader(vsCode, version);
    m_simpleShader->attachFragmentShader(fsCode, version);
    m_simpleShader->link();
}

void TextureMapping::initBlendShader() {
    const char* version =
#ifdef USE_GLES
        "300 es"
#else
        "330 core"
#endif
        ;

    const char* vsCode =
        "layout(location = 0) in vec3 aPosition;\n"
        "layout(location = 1) in vec3 aNormal;\n"
        "layout(location = 2) in vec2 aTexCoord;\n"

        "out vec3 fPosition;\n"
        "out vec3 fNormal;\n"
        "out vec2 fTexCoord;\n"

        "uniform mat4 projection;\n"
        "uniform mat4 view;\n"
        "uniform mat4 model;\n"

        "void main() {\n"
        "    fPosition = vec3(model * vec4(aPosition, 1.0f));\n"
        "    fNormal = mat3(transpose(inverse(model))) * aNormal;\n"
        "    fTexCoord = aTexCoord;\n"
        "    gl_Position = projection * view * model * vec4(aPosition, 1.0f);\n"
        "}\n";

    // TODO: change the fragment shader code to achieve the following goals
    // + blend the colors of the two textures
    // + lambert shading, i.e the color is affected by the light
    // write your code here
    // -----------------------------------------------------------------
    const char* fsCode =
        "in vec3 fPosition;\n"
        "in vec3 fNormal;\n"
        "in vec2 fTexCoord;\n"
        "out vec4 color;\n"

        "struct DirectionalLight {\n"
        "    vec3 direction;\n"
        "    vec3 color;\n"
        "    float intensity;\n"
        "};\n"

        "struct Material {\n"
        "    vec3 kds[2];\n"
        "    float blend;\n"
        "};\n"

        "uniform Material material;\n"
        "uniform DirectionalLight light;\n"
        "uniform sampler2D mapKds[2];\n"

        "void main() {\n"
        "    color = vec4(material.kds[0], 1.0f);\n"
        "}\n";
    //----------------------------------------------------------------

    m_blendShader.reset(new GLSLProgram);
    m_blendShader->attachVertexShader(vsCode, version);
    m_blendShader->attachFragmentShader(fsCode, version);
    m_blendShader->link();
}

void TextureMapping::initCheckerShader() {
    const char* version =
#ifdef USE_GLES
        "300 es"
#else
        "330 core"
#endif
        ;

    const char* vsCode =
        "layout(location = 0) in vec3 aPosition;\n"
        "layout(location = 1) in vec3 aNormal;\n"
        "layout(location = 2) in vec2 aTexCoord;\n"
        "out vec2 fTexCoord;\n"
        "uniform mat4 projection;\n"
        "uniform mat4 view;\n"
        "uniform mat4 model;\n"
        "void main() {\n"
        "    fTexCoord = aTexCoord;\n"
        "    gl_Position = projection * view * model * vec4(aPosition, 1.0f);\n"
        "}\n";

    // TODO: change the following code to achieve the procedural checker texture
    // hint: use the fTexCoord to determine the color
    // modify your code here
    // --------------------------------------------------------------
    const char* fsCode =
        "in vec2 fTexCoord;\n"
        "out vec4 color;\n"

        "struct Material {\n"
        "    vec3 colors[2];\n"
        "    int repeat;\n"
        "};\n"

        "uniform Material material;\n"

        "void main() {\n"
        "    color = vec4(material.colors[0], 1.0f);\n"
        "}\n";
    //----------------------------------------------------------------

    m_checkerShader.reset(new GLSLProgram);
    m_checkerShader->attachVertexShader(vsCode, version);
    m_checkerShader->attachFragmentShader(fsCode, version);
    m_checkerShader->link();
}

void TextureMapping::handleInput() {
    if (m_input.keyboard.keyStates[GLFW_KEY_ESCAPE] != GLFW_RELEASE) {
        glfwSetWindowShouldClose(m_window, true);
        return;
    }

    const float angluarVelocity = 0.1f;
    const float angle = angluarVelocity * static_cast<float>(m_deltaTime);
    const glm::vec3 axis = glm::vec3(0.0f, 1.0f, 0.0f);
    m_sphere->transform.rotation = glm::angleAxis(angle, axis) * m_sphere->transform.rotation;
}

void TextureMapping::renderFrame() {
    // some options related to imGUI
    static bool wireframe = false;

    // trivial things
    showFpsInWindowTitle();

    glClearColor(m_clearColor.r, m_clearColor.g, m_clearColor.b, m_clearColor.a);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);

#ifndef USE_GLES
    if (wireframe) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    } else {
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }
#endif

    const glm::mat4 projection = m_camera->getProjectionMatrix();
    const glm::mat4 view = m_camera->getViewMatrix();

    // draw planet
    switch (m_renderMode) {
    case RenderMode::Simple:
        // 1. use the shader
        m_simpleShader->use();
        // 2. transfer mvp matrices to gpu
        m_simpleShader->setUniformMat4("projection", projection);
        m_simpleShader->setUniformMat4("view", view);
        m_simpleShader->setUniformMat4("model", m_sphere->transform.getLocalMatrix());
        // 3. enable textures and transform textures to gpu
        m_simpleMaterial->mapKd->bind();
        break;
    case RenderMode::Blend:
        // 1. use the shader
        m_blendShader->use();
        // 2. transfer mvp matrices to gpu
        m_blendShader->setUniformMat4("projection", projection);
        m_blendShader->setUniformMat4("view", view);
        m_blendShader->setUniformMat4("model", m_sphere->transform.getLocalMatrix());
        // 3. transfer light attributes to gpu
        m_blendShader->setUniformVec3("light.direction", m_light->transform.getFront());
        m_blendShader->setUniformVec3("light.color", m_light->color);
        m_blendShader->setUniformFloat("light.intensity", m_light->intensity);
        // 4. transfer materials to gpu
        // 4.1 transfer simple material attributes
        m_blendShader->setUniformVec3("material.kds[0]", m_blendMaterial->kds[0]);
        m_blendShader->setUniformVec3("material.kds[1]", m_blendMaterial->kds[1]);
        // 4.2 transfer blend cofficient to gpu
        m_blendShader->setUniformFloat("material.blend", m_blendMaterial->blend);
        // 4.3 TODO: enable textures and transform textures to gpu
        // write your code here
        //----------------------------------------------------------------
        // ...
        //----------------------------------------------------------------

        break;
    case RenderMode::Checker:
        // 1. use the shader
        m_checkerShader->use();
        // 2. transfer mvp matrices to gpu
        m_checkerShader->setUniformMat4("projection", projection);
        m_checkerShader->setUniformMat4("view", view);
        m_checkerShader->setUniformMat4("model", m_sphere->transform.getLocalMatrix());
        // 3. transfer material attributes to gpu
        m_checkerShader->setUniformInt("material.repeat", m_checkerMaterial->repeat);
        m_checkerShader->setUniformVec3("material.colors[0]", m_checkerMaterial->colors[0]);
        m_checkerShader->setUniformVec3("material.colors[1]", m_checkerMaterial->colors[1]);
        break;
    }

    m_sphere->draw();

    // draw skybox
    m_skybox->draw(projection, view);

    // draw ui elements
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    const auto flags = ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings;

    if (!ImGui::Begin("Control Panel", nullptr, flags)) {
        ImGui::End();
    } else {
        ImGui::Text("Render Mode");
        ImGui::Separator();
        ImGui::RadioButton("Simple Texture Shading", (int*)&m_renderMode, (int)(RenderMode::Simple));
        ImGui::NewLine();

        ImGui::RadioButton("Blend Texture Shading", (int*)&m_renderMode, (int)(RenderMode::Blend));
        ImGui::ColorEdit3("kd1", (float*)&m_blendMaterial->kds[0]);
        ImGui::ColorEdit3("kd2", (float*)&m_blendMaterial->kds[1]);
        ImGui::SliderFloat("blend", &m_blendMaterial->blend, 0.0f, 1.0f);
        ImGui::NewLine();

        ImGui::RadioButton("Checker Shading", (int*)&m_renderMode, (int)(RenderMode::Checker));
        ImGui::SliderInt("repeat", &m_checkerMaterial->repeat, 2, 20);
        ImGui::ColorEdit3("color1", (float*)&m_checkerMaterial->colors[0]);
        ImGui::ColorEdit3("color2", (float*)&m_checkerMaterial->colors[1]);
#ifndef __EMSCRIPTEN__
        ImGui::Checkbox("wireframe", &wireframe);
#endif
        ImGui::NewLine();

        ImGui::Text("Directional light");
        ImGui::Separator();
        ImGui::SliderFloat("intensity", &m_light->intensity, 0.0f, 2.0f);
        ImGui::ColorEdit3("color", (float*)&m_light->color);
        ImGui::NewLine();

        ImGui::End();
    }

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}