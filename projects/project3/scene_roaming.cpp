#include "scene_roaming.h"

const std::string modelRelPath = "obj/bunny.obj";

SceneRoaming::SceneRoaming(const Options& options) : Application(options) {
    // set input mode
    glfwSetInputMode(m_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    m_input.mouse.move.xNow = m_input.mouse.move.xOld = 0.5f * m_windowWidth;
    m_input.mouse.move.yNow = m_input.mouse.move.yOld = 0.5f * m_windowHeight;
    glfwSetCursorPos(m_window, m_input.mouse.move.xNow, m_input.mouse.move.yNow);

    // init cameras
    m_cameras.resize(2);

    const float aspect = 1.0f * m_windowWidth / m_windowHeight;
    constexpr float znear = 0.1f;
    constexpr float zfar = 10000.0f;

    // perspective camera
    m_cameras[0].reset(new PerspectiveCamera(glm::radians(60.0f), aspect, 0.1f, 10000.0f));
    m_cameras[0]->transform.position = glm::vec3(0.0f, 0.0f, 15.0f);

    // orthographic camera
    m_cameras[1].reset(
        new OrthographicCamera(-4.0f * aspect, 4.0f * aspect, -4.0f, 4.0f, znear, zfar));
    m_cameras[1]->transform.position = glm::vec3(0.0f, 0.0f, 15.0f);

    // init model
    m_bunny.reset(new Model(getAssetFullPath(modelRelPath)));

    std::vector<Vertex> vertices{
        //         position         |        normal        |   texcoord
        {{-5.0f, 0.0f, -5.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
        { {5.0f, 0.0f, -5.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
        {  {5.0f, 0.0f, 5.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 1.0f}},
        { {-5.0f, 0.0f, 5.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 1.0f}}
    };
    std::vector<uint32_t> indices{0, 2, 1, 0, 3, 2};

    m_ground.reset(new Model(vertices, indices));

    // init shader
    initShader();
}

void SceneRoaming::handleInput() {
    constexpr float cameraMoveSpeed = 5.0f;
    constexpr float cameraRotateSpeed = 0.02f;

    if (m_input.keyboard.keyStates[GLFW_KEY_ESCAPE] != GLFW_RELEASE) {
        glfwSetWindowShouldClose(m_window, true);
        return;
    }

    if (m_input.keyboard.keyStates[GLFW_KEY_SPACE] == GLFW_PRESS) {
        std::cout << "switch camera" << std::endl;
        // switch camera
        m_activeCameraIndex = (m_activeCameraIndex + 1) % m_cameras.size();
        m_input.keyboard.keyStates[GLFW_KEY_SPACE] = GLFW_RELEASE;
        return;
    }

    Camera* camera = m_cameras[m_activeCameraIndex].get();

    if (m_input.keyboard.keyStates[GLFW_KEY_W] != GLFW_RELEASE) {
        std::cout << "W" << std::endl;
        // TODO: move the camera in its front direction
        // write your code here
        // -------------------------------------------------
        // camera->transform.position = ...;
        // -------------------------------------------------
    }

    if (m_input.keyboard.keyStates[GLFW_KEY_A] != GLFW_RELEASE) {
        std::cout << "A" << std::endl;
        // TODO: move the camera in its left direction
        // write your code here
        // -------------------------------------------------
        // camera->transform.position = ...;
        // -------------------------------------------------
    }

    if (m_input.keyboard.keyStates[GLFW_KEY_S] != GLFW_RELEASE) {
        std::cout << "S" << std::endl;
        // TODO: move the camera in its back direction
        // write your code here
        // -------------------------------------------------
        // camera->transform.position = ...;
        // -------------------------------------------------
    }

    if (m_input.keyboard.keyStates[GLFW_KEY_D] != GLFW_RELEASE) {
        std::cout << "D" << std::endl;
        // TODO: move the camera in its right direction
        // write your code here
        // -------------------------------------------------
        // camera->transform.position = ...;
        // -------------------------------------------------
    }

    if (m_input.mouse.move.xNow != m_input.mouse.move.xOld) {
        std::cout << "mouse move in x direction" << std::endl;
        // TODO: rotate the camera around world up: glm::vec3(0.0f, 1.0f, 0.0f)
        // hint1: you should know how do quaternion work to represent rotation
        // hint2: mouse_movement_in_x_direction = m_input.mouse.move.xNow - m_input.mouse.move.xOld
        // write your code here
        // -----------------------------------------------------------------------------
        // camera->transform.rotation = ...
        // -----------------------------------------------------------------------------
    }

    if (m_input.mouse.move.yNow != m_input.mouse.move.yOld) {
        std::cout << "mouse move in y direction" << std::endl;
        // TODO: rotate the camera around its local right
        // hint1: you should know how do quaternion work to represent rotation
        // hint2: mouse_movement_in_y_direction = m_input.mouse.move.yNow - m_input.mouse.move.yOld
        // write your code here
        // -----------------------------------------------------------------------------
        // camera->transform.rotation = ...
        // -----------------------------------------------------------------------------
    }

    m_input.forwardState();
}

void SceneRoaming::renderFrame() {
    showFpsInWindowTitle();

    glClearColor(m_clearColor.r, m_clearColor.g, m_clearColor.b, m_clearColor.a);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);

    glm::mat4 projection = m_cameras[m_activeCameraIndex]->getProjectionMatrix();
    glm::mat4 view = m_cameras[m_activeCameraIndex]->getViewMatrix();

    m_shader->use();
    m_shader->setUniformMat4("projection", projection);
    m_shader->setUniformMat4("view", view);
    m_shader->setUniformMat4("model", m_bunny->transform.getLocalMatrix());

    m_bunny->draw();

    m_shader->setUniformMat4("model", glm::translate(glm::mat4(1.0f), glm::vec3(0, -2.25, 0)));
    m_ground->draw();
}

void SceneRoaming::initShader() {
    const char* vsCode =
        "#version 330 core\n"
        "layout(location = 0) in vec3 aPosition;\n"
        "layout(location = 1) in vec3 aNormal;\n"

        "out vec3 worldPosition;\n"
        "out vec3 normal;\n"

        "uniform mat4 model;\n"
        "uniform mat4 view;\n"
        "uniform mat4 projection;\n"

        "void main() {\n"
        "    normal = mat3(transpose(inverse(model))) * aNormal;\n"
        "    worldPosition = vec3(model * vec4(aPosition, 1.0f));\n"
        "    gl_Position = projection * view * vec4(worldPosition, 1.0f);\n"
        "}\n";

    const char* fsCode =
        "#version 330 core\n"
        "in vec3 worldPosition;\n"
        "in vec3 normal;\n"
        "out vec4 fragColor;\n"

        "void main() {\n"
        "    vec3 lightPosition = vec3(100.0f, 100.0f, 100.0f);\n"
        "    // ambient color\n"
        "    float ka = 0.1f;\n"
        "    vec3 objectColor = vec3(1.0f, 1.0f, 1.0f);\n"
        "    vec3 ambient = ka * objectColor;\n"
        "    // diffuse color\n"
        "    float kd = 0.8f;\n"
        "    vec3 lightColor = vec3(1.0f, 1.0f, 1.0f);\n"
        "    vec3 lightDirection = normalize(lightPosition - worldPosition);\n"
        "    vec3 diffuse = kd * lightColor * max(dot(normalize(normal), lightDirection), 0.0f);\n"
        "    fragColor = vec4(ambient + diffuse, 1.0f);\n"
        "}\n";

    m_shader.reset(new GLSLProgram);
    m_shader->attachVertexShader(vsCode);
    m_shader->attachFragmentShader(fsCode);
    m_shader->link();
}