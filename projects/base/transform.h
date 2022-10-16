#pragma once

#include <vector>

#include <glm/glm.hpp>
#include <glm/ext.hpp>

struct Transform {
public:
	glm::vec3 position = { 0.0f, 0.0f, 0.0f };
	glm::quat rotation = { 1.0f, 0.0f, 0.0f, 0.0f };
	glm::vec3 scale = { 1.0f, 1.0f, 1.0f };
public:
	void setFromTRS(const glm::mat4& trs);

	void lookAt(const glm::vec3& target, const glm::vec3& up = getDefaultUp());

	glm::vec3 getFront() const;

	glm::vec3 getUp() const;

	glm::vec3 getRight() const;

	glm::mat4 getLocalMatrix() const;

	static constexpr glm::vec3 getDefaultFront() {
		return { 0.0f, 0.0f, -1.0f };
	}

	static constexpr glm::vec3 getDefaultUp() {
		return { 0.0f, 1.0f, 0.0f };
	}

	static constexpr glm::vec3 getDefaultRight() {
		return { 1.0f, 0.0f, 0.0f };
	}
};