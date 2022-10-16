#pragma once

#include "transform.h"

struct Light {
	Transform transform;
	float intensity = 1.0f;
	glm::vec3 color = { 1.0f, 1.0f, 1.0f };
};

struct AmbientLight : public Light {

};

struct DirectionalLight : public Light{

};

struct PointLight : public Light {
	float kc = 1.0f;
	float kl = 0.0f;
	float kq = 1.0f;
};

struct SpotLight : public Light {
	float angle = glm::radians(60.0f);
	float kc = 1.0f;
	float kl = 0.0f;
	float kq = 1.0f;
};