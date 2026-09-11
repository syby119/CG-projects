#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include "shading_tutorial.h"

const std::string modelRelPath = "obj/bunny.obj";

ShadingTutorial::ShadingTutorial(const Options& options) : Application(options) {
    // init model
    m_bunny.reset(new Model(getAssetFullPath(modelRelPath)));

    // init materials
    m_ambientMaterial.reset(new AmbientMaterial);
    m_ambientMaterial->ka = glm::vec3(0.03f, 0.03f, 0.03f);

    m_lambertMaterial.reset(new LambertMaterial);
    m_lambertMaterial->kd = glm::vec3(1.0f, 1.0f, 1.0f);

    m_phongMaterial.reset(new PhongMaterial);
    m_phongMaterial->ka = glm::vec3(0.03f, 0.03f, 0.03f);
    m_phongMaterial->kd = glm::vec3(1.0f, 1.0f, 1.0f);
    m_phongMaterial->ks = glm::vec3(1.0f, 1.0f, 1.0f);
    m_phongMaterial->ns = 10.0f;

    // init shaders
    initAmbientShader();
    initLambertShader();
    initPhongShader();

    // init lights
    m_ambientLight.reset(new AmbientLight);

    m_directionalLight.reset(new DirectionalLight);
    m_directionalLight->intensity = 0.5f;
    m_directionalLight->transform.rotation =
        glm::angleAxis(glm::radians(45.0f), glm::normalize(glm::vec3(-1.0f)));

    m_spotLight.reset(new SpotLight);
    m_spotLight->intensity = 0.5f;
    m_spotLight->angle = glm::radians(90.0f);
    m_spotLight->transform.position = glm::vec3(0.0f, 0.0f, 2.5f);
    m_spotLight->transform.rotation = glm::vec3(0.0f, 0.0f, 0.0f);

    // init camera
    m_camera.reset(new PerspectiveCamera(
        glm::radians(50.0f), 1.0f * m_windowWidth / m_windowHeight, 0.1f, 1000.0f));
    m_camera->transform.position.z = 10.0f;

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

ShadingTutorial::~ShadingTutorial() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

void ShadingTutorial::initAmbientShader() {
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
        "uniform mat4 model;\n"
        "uniform mat4 view;\n"
        "uniform mat4 projection;\n"
        "void main() {\n"
        "    gl_Position = projection * view * model * vec4(aPosition, 1.0f);\n"
        "}\n";

    const char* fsCode =
        "out vec4 color;\n"

        "// material data structure declaration\n"
        "struct Material {\n"
        "    vec3 ka;\n"
        "};\n"

        "// ambient light data structure declaration\n"
        "struct AmbientLight {\n"
        "    vec3 color;\n"
        "    float intensity;\n"
        "};\n"

        "// uniform variables\n"
        "uniform Material material;\n"
        "uniform AmbientLight ambientLight;\n"

        "void main() {\n"
        "    vec3 ambient = material.ka * ambientLight.color * ambientLight.intensity;\n"
        "    color = vec4(ambient, 1.0f);\n"
        "}\n";

    m_ambientShader.reset(new GLSLProgram);
    m_ambientShader->attachVertexShader(vsCode, version);
    m_ambientShader->attachFragmentShader(fsCode, version);
    m_ambientShader->link();
}

void ShadingTutorial::initLambertShader() {
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

        "uniform mat4 model;\n"
        "uniform mat4 view;\n"
        "uniform mat4 projection;\n"

        "void main() {\n"
        "    fPosition = vec3(model * vec4(aPosition, 1.0f));\n"
        "    fNormal = mat3(transpose(inverse(model))) * aNormal;\n"
        "    gl_Position = projection * view * model * vec4(aPosition, 1.0f);\n"
        "}\n";

    const char* fsCode =
        "in vec3 fPosition;\n"
        "in vec3 fNormal;\n"
        "out vec4 color;\n"

        "// material data structure declaration\n"
        "struct Material {\n"
        "    vec3 kd;\n"
        "};\n"

        "// directional light data structure declaration\n"
        "struct DirectionalLight {\n"
        "    vec3 direction;\n"
        "    float intensity;\n"
        "    vec3 color;\n"
        "};\n"

        "// spot light data structure declaration\n"
        "struct SpotLight {\n"
        "    vec3 position;\n"
        "    vec3 direction;\n"
        "    float intensity;\n"
        "    vec3 color;\n"
        "    float angle;\n"
        "    float kc;\n"
        "    float kl;\n"
        "    float kq;\n"
        "};\n"

        "// uniform variables\n"
        "uniform Material material;\n"
        "uniform DirectionalLight directionalLight;\n"
        "uniform SpotLight spotLight;\n"

        "vec3 calcDirectionalLight(vec3 normal) {\n"
        "    vec3 lightDir = normalize(-directionalLight.direction);\n"
        "    vec3 diffuse = directionalLight.color * max(dot(lightDir, normal), 0.0f) * "
        "material.kd;\n"
        "    return directionalLight.intensity * diffuse ;\n"
        "}\n"

        "vec3 calcSpotLight(vec3 normal) {\n"
        "    vec3 lightDir = normalize(spotLight.position - fPosition);\n"
        "    float theta = acos(-dot(lightDir, normalize(spotLight.direction)));\n"
        "    if (theta > spotLight.angle) {\n"
        "        return vec3(0.0f, 0.0f, 0.0f);\n"
        "    }\n"
        "    vec3 diffuse = spotLight.color * max(dot(lightDir, normal), 0.0f) * material.kd;\n"
        "    float distance = length(spotLight.position - fPosition);\n"
        "    float attenuation = 1.0f / (spotLight.kc + spotLight.kl * distance + spotLight.kq * "
        "distance * distance);\n"
        "    return spotLight.intensity * attenuation * diffuse;\n"
        "}\n"

        "void main() {\n"
        "    vec3 normal = normalize(fNormal);\n"
        "    vec3 diffuse = calcDirectionalLight(normal) + calcSpotLight(normal);\n"
        "    color = vec4(diffuse, 1.0f);\n"
        "}\n";

    m_lambertShader.reset(new GLSLProgram);
    m_lambertShader->attachVertexShader(vsCode, version);
    m_lambertShader->attachFragmentShader(fsCode, version);
    m_lambertShader->link();
}

void ShadingTutorial::initPhongShader() {
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

        "uniform mat4 model;\n"
        "uniform mat4 view;\n"
        "uniform mat4 projection;\n"

        "void main() {\n"
        "    fPosition = vec3(model * vec4(aPosition, 1.0f));\n"
        "    fNormal = mat3(transpose(inverse(model))) * aNormal;\n"
        "    gl_Position = projection * view * model * vec4(aPosition, 1.0f);\n"
        "}\n";

    // TODO: change the shader code for phong shading
    // hint1: you can copy the fragment shader code from ambient shader to get ambient term
    // hint2: you can copy the fragment shader code from lambert shader to get diffuse term
    // hint3: you should calculate the specular term by yourself
    // hint4: add up the ambient term, diffuse term and specular term, you can get the answer
    // ------------------------------------------------------------
    const char* fsCode =
        "out vec4 color;\n"
        "void main() {\n"
        "    color = vec4(1.0f, 1.0f, 1.0f, 1.0f);\n"
        "}\n";
    // ------------------------------------------------------------

    m_phongShader.reset(new GLSLProgram);
    m_phongShader->attachVertexShader(vsCode, version);
    m_phongShader->attachFragmentShader(fsCode, version);
    m_phongShader->link();
}

void ShadingTutorial::handleInput() {
    if (m_input.keyboard.keyStates[GLFW_KEY_ESCAPE] != GLFW_RELEASE) {
        glfwSetWindowShouldClose(m_window, true);
        return;
    }
}

void ShadingTutorial::renderFrame() {
    showFpsInWindowTitle();

    glClearColor(m_clearColor.r, m_clearColor.g, m_clearColor.b, m_clearColor.a);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);

    switch (m_renderMode) {
    case RenderMode::Ambient:
        m_ambientShader->use();
        // 1. transfer mvp matrix to the shader
        m_ambientShader->setUniformMat4("projection", m_camera->getProjectionMatrix());
        m_ambientShader->setUniformMat4("view", m_camera->getViewMatrix());
        m_ambientShader->setUniformMat4("model", m_bunny->transform.getLocalMatrix());
        // 2. transfer material attributes to the shader
        m_ambientShader->setUniformVec3("material.ka", m_ambientMaterial->ka);
        // 3. transfer light attributes to the shader
        m_ambientShader->setUniformVec3("ambientLight.color", m_ambientLight->color);
        m_ambientShader->setUniformFloat("ambientLight.intensity", m_ambientLight->intensity);
        break;
    case RenderMode::Lambert:
        m_lambertShader->use();
        // 1. transfer mvp matrix to the shader
        m_lambertShader->setUniformMat4("projection", m_camera->getProjectionMatrix());
        m_lambertShader->setUniformMat4("view", m_camera->getViewMatrix());
        m_lambertShader->setUniformMat4("model", m_bunny->transform.getLocalMatrix());
        // 2. transfer material attributes to the shader
        m_lambertShader->setUniformVec3("material.kd", m_lambertMaterial->kd);
        // 3. transfer light attributes to the shader
        m_lambertShader->setUniformVec3("spotLight.position", m_spotLight->transform.position);
        m_lambertShader->setUniformVec3("spotLight.direction", m_spotLight->transform.getFront());
        m_lambertShader->setUniformFloat("spotLight.intensity", m_spotLight->intensity);
        m_lambertShader->setUniformVec3("spotLight.color", m_spotLight->color);
        m_lambertShader->setUniformFloat("spotLight.angle", m_spotLight->angle);
        m_lambertShader->setUniformFloat("spotLight.kc", m_spotLight->kc);
        m_lambertShader->setUniformFloat("spotLight.kl", m_spotLight->kl);
        m_lambertShader->setUniformFloat("spotLight.kq", m_spotLight->kq);
        m_lambertShader->setUniformVec3(
            "directionalLight.direction", m_directionalLight->transform.getFront());
        m_lambertShader->setUniformFloat("directionalLight.intensity", m_directionalLight->intensity);
        m_lambertShader->setUniformVec3("directionalLight.color", m_directionalLight->color);
        break;
    case RenderMode::Phong:
        m_phongShader->use();
        // 1. transfer the mvp matrices to the shader
        m_phongShader->setUniformMat4("projection", m_camera->getProjectionMatrix());
        m_phongShader->setUniformMat4("view", m_camera->getViewMatrix());
        m_phongShader->setUniformMat4("model", m_bunny->transform.getLocalMatrix());

        // 2. TODO: transfer the camera position to the shader
        // write your code here
        // ----------------------------------------------------------------
        // m_phongShader->set...
        // ----------------------------------------------------------------

        // 3. TODO: transfer the material attributes to the shader
        // write your code here
        // -----------------------------------------------------------
        // m_phongShader->set...
        // -----------------------------------------------------------

        // 4. TODO: transfer the light attributes to the shader
        // write your code here
        // -----------------------------------------------------------
        // m_phongShader->set...
        // -----------------------------------------------------------

        break;
    }

    // draw the bunny
    m_bunny->draw();

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

        ImGui::RadioButton("ambient", (int*)&m_renderMode, (int)(RenderMode::Ambient));
        ImGui::ColorEdit3("ka##1", (float*)&m_ambientMaterial->ka);
        ImGui::NewLine();

        ImGui::RadioButton("lambert", (int*)&m_renderMode, (int)(RenderMode::Lambert));
        ImGui::ColorEdit3("kd##2", (float*)&m_lambertMaterial->kd);
        ImGui::NewLine();

        ImGui::RadioButton("phong", (int*)&m_renderMode, (int)(RenderMode::Phong));
        ImGui::ColorEdit3("ka##3", (float*)&m_phongMaterial->ka);
        ImGui::ColorEdit3("kd##3", (float*)&m_phongMaterial->kd);
        ImGui::ColorEdit3("ks##3", (float*)&m_phongMaterial->ks);
        ImGui::SliderFloat("ns##3", &m_phongMaterial->ns, 1.0f, 50.0f);
        ImGui::NewLine();

        ImGui::Text("ambient light");
        ImGui::Separator();
        ImGui::SliderFloat("intensity##1", &m_ambientLight->intensity, 0.0f, 1.0f);
        ImGui::ColorEdit3("color##1", (float*)&m_ambientLight->color);
        ImGui::NewLine();

        ImGui::Text("directional light");
        ImGui::Separator();
        ImGui::SliderFloat("intensity##2", &m_directionalLight->intensity, 0.0f, 1.0f);
        ImGui::ColorEdit3("color##2", (float*)&m_directionalLight->color);
        ImGui::NewLine();

        ImGui::Text("spot light");
        ImGui::Separator();
        ImGui::SliderFloat("intensity##3", &m_spotLight->intensity, 0.0f, 1.0f);
        ImGui::ColorEdit3("color##3", (float*)&m_spotLight->color);
        ImGui::SliderFloat(
            "angle##3", (float*)&m_spotLight->angle, 0.0f, glm::radians(180.0f), "%f rad");
        ImGui::NewLine();

        ImGui::End();
    }

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}