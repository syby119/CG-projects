#ifdef __EMSCRIPTEN__
    #include <emscripten.h>
#endif

#include "application.h"

Application::Application(const Options& options)
    : _assetRootDir(options.assetRootDir), _windowTitle(options.windowTitle),
      _windowWidth(options.windowWidth), _windowHeight(options.windowHeight),
      _clearColor(options.backgroundColor) {
    // set error callback
    glfwSetErrorCallback(errorCallback);

    // init glfw
    if (glfwInit() != GLFW_TRUE) {
        throw std::runtime_error("init glfw failure");
    }

    // create glfw window
#if defined(__EMSCRIPTEN__)
    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
#elif defined(USE_GLES)
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
#else
    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_API);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, options.glVersion.first);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, options.glVersion.second);
#endif

    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_RESIZABLE, options.windowResizable);

    if (options.msaa) {
        glfwWindowHint(GLFW_SAMPLES, 4);
    }

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    _window = glfwCreateWindow(_windowWidth, _windowHeight, _windowTitle.c_str(), nullptr, nullptr);

    if (_window == nullptr) {
        glfwTerminate();
        throw std::runtime_error("create glfw window failure");
    }

    glfwMakeContextCurrent(_window);
    glfwSetWindowUserPointer(_window, this);

#ifndef __EMSCRIPTEN__
    if (options.vSync) {
        glfwSwapInterval(1);
    } else {
        glfwSwapInterval(0);
    }
#endif

    // load OpenGL library functions
#ifdef __EMSCRIPTEN__
    // Emscripten link OpenGL ES library statically
#elif USE_GLES
    if (!gladLoadGLES2(glfwGetProcAddress)) {
        throw std::runtime_error("glad initialization OpenGL/ES2 failure");
    }
#else
    if (!gladLoadGL(glfwGetProcAddress)) {
        throw std::runtime_error("glad initialization OpenGL failure");
    }
#endif

    std::cout << "OpenGL\n";
    std::cout << "+ version:    " << glGetString(GL_VERSION) << '\n';
    std::cout << "+ renderer:   " << glGetString(GL_RENDERER) << '\n';
    std::cout << "+ glsl:       " << glGetString(GL_SHADING_LANGUAGE_VERSION) << '\n';
    std::cout << std::endl;

    // framebuffer and viewport
    glfwGetFramebufferSize(_window, &_windowWidth, &_windowHeight);
    glViewport(0, 0, _windowWidth, _windowHeight);

#ifndef USE_GLES
    if (options.msaa) {
        glEnable(GL_MULTISAMPLE);
    }
#endif

    // callback functions
    glfwSetFramebufferSizeCallback(_window, framebufferResizeCallback);
    glfwSetKeyCallback(_window, keyCallback);
    glfwSetMouseButtonCallback(_window, mouseButtonCallback);
    glfwSetCursorPosCallback(_window, cursorPosCallback);
    glfwSetScrollCallback(_window, scrollCallback);

    // register mainloop for WebGL
#ifdef __EMSCRIPTEN__
    if (mainloopRegisterFunc) {
        throw std::logic_error(
            "Application is designed to be singleton, though not implemented as is");
    }

    mainloopRegisterFunc = std::bind(&Application::mainloop, this);
#endif

    // record time
    _lastTimeStamp = std::chrono::high_resolution_clock::now();
}

Application::~Application() {
    if (_window != nullptr) {
        glfwDestroyWindow(_window);
        _window = nullptr;
    }

    glfwTerminate();
}

void Application::run() {
#if __EMSCRIPTEN__
    emscripten_set_main_loop(mainloopWrapper, 0, 1);
#else
    while (!glfwWindowShouldClose(_window)) {
        mainloop();
    }
#endif
}

std::string Application::getAssetFullPath(const std::string& resourceRelPath) const {
    return _assetRootDir + resourceRelPath;
}

void Application::mainloop() {
    updateTime();
    handleInput();
    renderFrame();

    glfwSwapBuffers(_window);
    glfwPollEvents();
}

#ifdef __EMSCRIPTEN__
std::function<void()> Application::mainloopRegisterFunc;
void Application::mainloopWrapper() {
    mainloopRegisterFunc();
}
#endif

void Application::updateTime() {
    auto now = std::chrono::high_resolution_clock::now();
    _deltaTime = 0.001f * std::chrono::duration<float, std::milli>(now - _lastTimeStamp).count();
    _lastTimeStamp = now;
    if (_deltaTime != 0.0f) {
        _fpsIndicator.push(1.0f / _deltaTime);
    }
}

void Application::showFpsInWindowTitle() {
    float fps = _fpsIndicator.getAverageFrameRate();
    std::string detailTitle = _windowTitle + ": " + std::to_string(fps) + " fps";
    glfwSetWindowTitle(_window, detailTitle.c_str());
}

void Application::errorCallback(int error, const char* description) {
    std::cerr << description << std::endl;
}

void Application::framebufferResizeCallback(GLFWwindow* window, int width, int height) {
    Application* app = reinterpret_cast<Application*>(glfwGetWindowUserPointer(window));
    app->_windowWidth = width;
    app->_windowHeight = height;
    app->_windowReized = true;
    glViewport(0, 0, width, height);
}

void Application::cursorPosCallback(GLFWwindow* window, double xPos, double yPos) {
    Application* app = reinterpret_cast<Application*>(glfwGetWindowUserPointer(window));
    app->_input.mouse.move.xNow = static_cast<float>(xPos);
    app->_input.mouse.move.yNow = static_cast<float>(yPos);
}

void Application::mouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
    Application* app = reinterpret_cast<Application*>(glfwGetWindowUserPointer(window));
    if (action == GLFW_PRESS) {
        switch (button) {
        case GLFW_MOUSE_BUTTON_LEFT: app->_input.mouse.press.left = true; break;
        case GLFW_MOUSE_BUTTON_MIDDLE: app->_input.mouse.press.middle = true; break;
        case GLFW_MOUSE_BUTTON_RIGHT: app->_input.mouse.press.right = true; break;
        }
    } else if (action == GLFW_RELEASE) {
        switch (button) {
        case GLFW_MOUSE_BUTTON_LEFT: app->_input.mouse.press.left = false; break;
        case GLFW_MOUSE_BUTTON_MIDDLE: app->_input.mouse.press.middle = false; break;
        case GLFW_MOUSE_BUTTON_RIGHT: app->_input.mouse.press.right = false; break;
        }
    }
}

void Application::scrollCallback(GLFWwindow* window, double xOffset, double yOffset) {
    Application* app = reinterpret_cast<Application*>(glfwGetWindowUserPointer(window));
    app->_input.mouse.scroll.xOffset = static_cast<float>(xOffset);
    app->_input.mouse.scroll.yOffset = static_cast<float>(yOffset);
}

void Application::keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (key != GLFW_KEY_UNKNOWN) {
        Application* app = reinterpret_cast<Application*>(glfwGetWindowUserPointer(window));
        app->_input.keyboard.keyStates[key] = action;
    }
}