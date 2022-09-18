#pragma once

#include <array>
#include <iostream>
#include <GLFW/glfw3.h>

struct Input {
	struct Mouse {
		struct {
			bool left = false;
			bool middle = false;
			bool right = false;
		} press;

		struct {
			float xNow = 0.0f;
			float yNow = 0.0f;
			float xOld = 0.0f;
			float yOld = 0.0f;
		} move;

		struct {
			float xOffset = 0.0f;
			float yOffset = 0.0f;
		} scroll;
	} mouse;

	struct Keyboard {
		std::array<int, GLFW_KEY_LAST + 1> keyStates = { GLFW_RELEASE };
	} keyboard;

	void forwardState() {
		mouse.move.xOld = mouse.move.xNow;
		mouse.move.yOld = mouse.move.yNow;

		mouse.scroll.xOffset = 0.0f;
		mouse.scroll.yOffset = 0.0f;
	}
};