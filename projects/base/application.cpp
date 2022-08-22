#include "application.h"

Application::Application(const Options& options)
	: _windowTitle(options.windowTitle),
	  _windowWidth(options.windowWidth),
	  _windowHeight(options.windowHeight),
	  _clearColor(options.backgroundColor) {
	glfwSetErrorCallback(errorCallback);
	if (glfwInit() != GLFW_TRUE) {
		throw std::runtime_error("init glfw failure");
	}

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, options.glVersion.first);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, options.glVersion.second);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_RESIZABLE, options.windowResizable);

	if (options.msaa) {
		glfwWindowHint(GLFW_SAMPLES, 4);
	}

#ifdef __APPLE__
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

	_window = glfwCreateWindow(
		_windowWidth, _windowHeight, _windowTitle.c_str(), nullptr, nullptr);

	if (_window == nullptr) {
		glfwTerminate();
		throw std::runtime_error("create glfw window failure");
	}

	glfwMakeContextCurrent(_window);
	glfwSetWindowUserPointer(_window, this);

	if (options.vSync) {
		glfwSwapInterval(1);
	} else {
		glfwSwapInterval(0);
	}

	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
		throw std::runtime_error("initialize glad failure");
	}

	glfwGetFramebufferSize(_window, &_windowWidth, &_windowHeight);
	glViewport(0, 0, _windowWidth, _windowHeight);

	if (options.msaa) {
		glEnable(GL_MULTISAMPLE);
	}

	glfwSetFramebufferSizeCallback(_window, framebufferResizeCallback);
	glfwSetKeyCallback(_window, keyboardCallback);
	glfwSetMouseButtonCallback(_window, mouseClickedCallback);
	glfwSetCursorPosCallback(_window, cursorMovedCallback);
	glfwSetScrollCallback(_window, scrollCallback);

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
	while (!glfwWindowShouldClose(_window)) {
		updateTime();
		handleInput();
		renderFrame();

		glfwSwapBuffers(_window);
		glfwPollEvents();
	}
}

void Application::updateTime() {
	auto now = std::chrono::high_resolution_clock::now();
	_deltaTime = 0.001f * std::chrono::duration<float, std::milli>(now - _lastTimeStamp).count();
	_lastTimeStamp = now;
	_fpsIndicator.push(1.0f / _deltaTime);
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

void Application::cursorMovedCallback(GLFWwindow* window, double xPos, double yPos) {
	Application* app = reinterpret_cast<Application*>(glfwGetWindowUserPointer(window));
	app->_mouseInput.move.xCurrent = xPos;
	app->_mouseInput.move.yCurrent = yPos;
}

void Application::mouseClickedCallback(GLFWwindow* window, int button, int action, int mods) {
	Application* app = reinterpret_cast<Application*>(glfwGetWindowUserPointer(window));
	if (action == GLFW_PRESS) {
		switch (button) {
			case GLFW_MOUSE_BUTTON_LEFT:
				app->_mouseInput.click.left = true;
				break;
			case GLFW_MOUSE_BUTTON_MIDDLE:
				app->_mouseInput.click.middle = true;
				break;
			case GLFW_MOUSE_BUTTON_RIGHT:
				app->_mouseInput.click.right = true;
				break;
		}
	} else if (action == GLFW_RELEASE) {
		switch (button) {
		case GLFW_MOUSE_BUTTON_LEFT:
			app->_mouseInput.click.left = false;
			break;
		case GLFW_MOUSE_BUTTON_MIDDLE:
			app->_mouseInput.click.middle = false;
			break;
		case GLFW_MOUSE_BUTTON_RIGHT:
			app->_mouseInput.click.right = false;
			break;
		}
	}
}

void Application::scrollCallback(GLFWwindow* window, double xOffset, double yOffset) {
	Application* app = reinterpret_cast<Application*>(glfwGetWindowUserPointer(window));
	app->_mouseInput.scroll.x += xOffset;
	app->_mouseInput.scroll.y += yOffset;
}

void Application::keyboardCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
	if (key != GLFW_KEY_UNKNOWN) {
		Application* app = reinterpret_cast<Application*>(glfwGetWindowUserPointer(window));
		app->_keyboardInput.keyStates[key] = action;
	}
}