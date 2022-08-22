#pragma once

#include <array>
#include <iostream>
#include <GLFW/glfw3.h>

struct KeyboardInput {
	std::array<int, GLFW_KEY_LAST + 1> keyStates = { GLFW_RELEASE };
};

struct MouseInput {
	struct {
		bool left = false;
		bool middle = false;
		bool right = false;
	} click;

	struct {
		double xOld = -1.0;
		double yOld = -1.0;
		double xCurrent = -1.0;
		double yCurrent = -1.0;
	} move;

	struct {
		double x = 0.0;
		double y = 0.0;
	} scroll;
};