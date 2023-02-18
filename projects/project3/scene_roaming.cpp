#include "scene_roaming.h"

const std::string modelRelPath = "obj/bunny.obj";

SceneRoaming::SceneRoaming(const Options& options): Application(options) {
    // set input mode
    glfwSetInputMode(_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    _input.mouse.move.xNow = _input.mouse.move.xOld = 0.5f * _windowWidth;
    _input.mouse.move.yNow = _input.mouse.move.yOld = 0.5f * _windowHeight;
    glfwSetCursorPos(_window, _input.mouse.move.xNow, _input.mouse.move.yNow);

    // init cameras
    _cameras.resize(2);

    const float aspect = 1.0f * _windowWidth / _windowHeight;
    constexpr float znear = 0.1f;
    constexpr float zfar = 10000.0f;

    // perspective camera
    _cameras[0].reset(new PerspectiveCamera(
        glm::radians(60.0f), aspect, 0.1f, 10000.0f));
    _cameras[0]->transform.position = glm::vec3(0.0f, 0.0f, 15.0f);

    // orthographic camera
    _cameras[1].reset(new OrthographicCamera(
        -4.0f * aspect, 4.0f * aspect, -4.0f, 4.0f, znear, zfar));
    _cameras[1]->transform.position = glm::vec3(0.0f, 0.0f, 15.0f);

    // init model
    _bunny.reset(new Model(getAssetFullPath(modelRelPath)));

    // init shader
    initShader();
}

void SceneRoaming::handleInput() {
    constexpr float cameraMoveSpeed = 5.0f;
    constexpr float cameraRotateSpeed = 0.02f;

    if (_input.keyboard.keyStates[GLFW_KEY_ESCAPE] != GLFW_RELEASE) {
        glfwSetWindowShouldClose(_window, true);
        return ;
    }

    if (_input.keyboard.keyStates[GLFW_KEY_SPACE] == GLFW_PRESS) {
        std::cout << "switch camera" << std::endl;
        // switch camera
        activeCameraIndex = (activeCameraIndex + 1) % _cameras.size();
        _input.keyboard.keyStates[GLFW_KEY_SPACE] = GLFW_RELEASE;
        return;
    }

    Camera* camera = _cameras[activeCameraIndex].get();

    if (_input.keyboard.keyStates[GLFW_KEY_W] != GLFW_RELEASE) {
        std::cout << "W" << std::endl;
        // TODO: move the camera in its front direction
        // write your code here
        // -------------------------------------------------
        // camera->transform.position = ...;
        // -------------------------------------------------
    }

    if (_input.keyboard.keyStates[GLFW_KEY_A] != GLFW_RELEASE) {
        std::cout << "A" << std::endl;
        // TODO: move the camera in its left direction
        // write your code here
        // -------------------------------------------------
        // camera->transform.position = ...;
        // -------------------------------------------------
    }

    if (_input.keyboard.keyStates[GLFW_KEY_S] != GLFW_RELEASE) {
        std::cout << "S" << std::endl;
        // TODO: move the camera in its back direction
        // write your code here
        // -------------------------------------------------
        // camera->transform.position = ...;
        // -------------------------------------------------
    }

    if (_input.keyboard.keyStates[GLFW_KEY_D] != GLFW_RELEASE) {
        std::cout << "D" << std::endl;
        // TODO: move the camera in its right direction
        // write your code here
        // -------------------------------------------------
        // camera->transform.position = ...;
        // -------------------------------------------------
    }

    if (_input.mouse.move.xNow != _input.mouse.move.xOld) {
        std::cout << "mouse move in x direction" << std::endl;
        // TODO: rotate the camera around world up: glm::vec3(0.0f, 1.0f, 0.0f)
        // hint1: you should know how do quaternion work to represent rotation
        // hint2: mouse_movement_in_x_direction = _input.mouse.move.xNow - _input.mouse.move.xOld
        // write your code here
        // -----------------------------------------------------------------------------
        // camera->transform.rotation = ...
        // -----------------------------------------------------------------------------
    }

    if (_input.mouse.move.yNow != _input.mouse.move.yOld) {
        std::cout << "mouse move in y direction" << std::endl;
        // TODO: rotate the camera around its local right
        // hint1: you should know how do quaternion work to represent rotation
        // hint2: mouse_movement_in_y_direction = _input.mouse.move.yNow - _input.mouse.move.yOld
        // write your code here
        // -----------------------------------------------------------------------------
        // camera->transform.rotation = ...
        // -----------------------------------------------------------------------------
    }

    _input.forwardState();
}

void SceneRoaming::renderFrame() {
    showFpsInWindowTitle();

    glClearColor(_clearColor.r, _clearColor.g, _clearColor.b, _clearColor.a);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);

    glm::mat4 projection = _cameras[activeCameraIndex]->getProjectionMatrix();
    glm::mat4 view = _cameras[activeCameraIndex]->getViewMatrix();
    
    _shader->use();
    _shader->setUniformMat4("projection", projection);
    _shader->setUniformMat4("view", view);
    _shader->setUniformMat4("model", _bunny->transform.getLocalMatrix());

    _bunny->draw();
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

    _shader.reset(new GLSLProgram);
    _shader->attachVertexShader(vsCode);
    _shader->attachFragmentShader(fsCode);
    _shader->link();
}