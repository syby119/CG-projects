#pragma once

#include <chrono>
#include <string>
#include <stdexcept>

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

#include "input.h"
#include "frame_rate_indicator.h"

struct Options {
	std::string assetRootDir;
	std::string windowTitle;
	int windowWidth;
	int windowHeight;
	bool windowResizable;
	bool vSync;
	bool msaa;
	std::pair<int, int> glVersion;
	glm::vec4 backgroundColor;
};

class Application {
public:
	Application(const Options& options);

	Application(const Application& rhs) = delete;

	Application(Application&& rhs) = delete;

	virtual ~Application();

	void run();

protected:
	/* _assetPath */
	std::string _assetRootDir;

	/* window info */
	GLFWwindow* _window = nullptr;
	std::string _windowTitle;
	int _windowWidth = 0;
	int _windowHeight = 0;
	bool _windowReized = false;

	/* timer for fps */
	std::chrono::time_point<std::chrono::high_resolution_clock> _lastTimeStamp;
	float _deltaTime = 0.0f;
	FrameRateIndicator _fpsIndicator{ 64 };

	/* input handler */
	Input _input;

	/* clear color */
	glm::vec4 _clearColor = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);

	std::string getAssetFullPath(const std::string& resourceRelPath) const;

	void updateTime();

	/* derived class can override this function to handle input */
	virtual void handleInput() = 0;

	/* derived class can override this function to render a frame */
	virtual void renderFrame() = 0;

	void showFpsInWindowTitle();

	static void errorCallback(int error, const char* description);

	static void framebufferResizeCallback(GLFWwindow* window, int width, int height);

	static void cursorPosCallback(GLFWwindow* window, double xPos, double yPos);

	static void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods);

	static void scrollCallback(GLFWwindow* window, double xOffset, double yOffset);

	static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
};